#pragma once
#include "axion/concept.hpp"

// Each relation is a five-plane projection onto 32 object cells. The same
// physical contacts can be traversed forwards or excited from their outputs.
namespace axion::life::meaning {
AX_FN bool unique(Word v) {return v && !(v&(v-1));}
AX_FN Activation uncertain(Word domain) {return {domain,1};}
AX_FN Word identifier(Activation v) {return !v.gap && unique(v.cells) ? Word(__builtin_ctz(v.cells)) : hidden_link;}
AX_FN Word domain(Word n) {return n==32 ? ~0u : (1u<<n)-1;}
AX_FN void arrange(Concepts &c,Word nodes,bool rename) {
    c.domain=domain(nodes);c.views[0]={};c.views[1]={};
    if (rename) for (Word i=0;i<concept_nodes;++i) {c.nouns[i]=0;c.referents[i]=0;}
}
AX_FN void store(Fibres &f,Word x,Word y) {
    Word bit=1u<<x;
    f.seen=(f.seen&~bit)|(y==hidden_link ? 0 : bit);
    f.connected=(f.connected&~bit)|(y<32 ? bit : 0);
    for (Word p=0;p<5;++p) f.target[p]=(f.target[p]&~bit)|((y<32 && (y&(1u<<p))) ? bit : 0);
}
AX_FN void observe(Concepts &c,Word r,Word x,Word retinal,bool peer_looking,bool heard=false) {
    if (r>=concept_channels || x>=32 || !(c.domain&(1u<<x)) || (retinal<32 && !(c.domain&(1u<<retinal)))) return;
    Word cue=Word(retinal!=hidden_link)+2*Word(peer_looking)+4*Word(heard);
    for (Word b=0;b<2;++b) {
        Word op=(c.genes.low>>(4*cue+2*b))&3;
        if (op==2) store(c.views[b].relation[r],x,hidden_link);
        else if (op==3 || (op==1 && retinal!=hidden_link)) store(c.views[b].relation[r],x,retinal);
    }
}
// A simultaneous display supplies the same retinal event at every port.
// Applying its write gate once per field avoids 32 read-modify-writes.
AX_FN void observe_field(Concepts &c,Word r,const Fibres &field,bool visible,bool other) {
    Word cue=Word(visible)+2*Word(other);
    for (Word b=0;b<2;++b) {
        Word op=(c.genes.low>>(4*cue+2*b))&3;
        if (op==2 || (op==3 && !visible)) c.views[b].relation[r]={};
        else if (visible && (op==1 || op==3)) c.views[b].relation[r]=field;
    }
}
AX_FN Word target(const Fibres &f,Word x) {
    Word index=0;for (Word p=0;p<5;++p) index|=((f.target[p]>>x)&1)<<p;
    return index;
}
AX_FN Word inputs(const Fibres &f,Word y) {
    Word v=f.connected;
    for (Word p=0;p<5;++p) v&=(y&(1u<<p)) ? f.target[p] : ~f.target[p];
    return v;
}
AX_FN Activation step(const RelationalView &v,Word all,Word fibre,Activation initial) {
    const auto &f=v.relation[fibre&3];Word active=initial.cells,result=0,gap=initial.gap;
    if (fibre<4) {
        if (active&~f.seen) return uncertain(all);
        gap|=bool(active&~f.connected);active&=f.connected;
        while (active) {Word x=Word(__builtin_ctz(active));active&=active-1;result|=1u<<target(f,x);}
    } else {
        Word unseen=all&~f.seen;
        while (active) {
            Word y=Word(__builtin_ctz(active));active&=active-1;
            Word back=inputs(f,y);result|=back|unseen;gap|=!back;
        }
    }
    return {result,gap};
}
AX_FN Count join(Count a,Count b) {
    if (!a || !b) return 0;
    Word shift=63-Word(__builtin_clzll(a));
    if (Word(__builtin_clzll(b))<shift) return 0;
    return (a^(1ull<<shift))|(b<<shift);
}
AX_FN Count atom(const Concepts &c,Word n) {return n<8 ? 8+n : n==15 ? 1 : c.words[n-8].walk;}
AX_FN Count wiring(const Concepts &c,Word code) {return join(atom(c,code&15),atom(c,code>>4));}
AX_FN Activation propagate(const RelationalView &v,Word all,Count route,Activation pulse) {
    if (!route) return uncertain(all);
    while (route>1 && pulse.cells) {pulse=step(v,all,Word(route)&7,pulse);route>>=3;}
    return pulse;
}
AX_FN Word word(const Concepts &c,Count form,bool acquire=false) {
    if (form) for (Word i=0;i<concept_words;++i) {
        if (c.words[i].form==form) return i;
        if (!c.words[i].form) return acquire ? i : concept_words;
    }
    return concept_words;
}
AX_FN Word noun_slot(const Concepts &c,Count form) {
    if (form) for (Word i=0;i<concept_nodes && c.nouns[i];++i) if (c.nouns[i]==form) return i;
    return concept_nodes;
}
AX_FN Word noun(const Concepts &c,Count form) {
    Word i=noun_slot(c,form);return i<concept_nodes ? c.referents[i]&c.domain : 0;
}
AX_FN Count pronounce(const Concepts &c,Activation v) {
    if (identifier(v)>=32) return 0;
    for (Word i=0;i<concept_nodes;++i) if (c.referents[i]==v.cells && c.nouns[i]) return c.nouns[i];
    return 0;
}
AX_FN bool point(Concepts &c,Count form,Word visible) {
    visible&=c.domain;if (!form || !visible) return false;
    Word k=0;while (k<concept_nodes && c.nouns[k] && c.nouns[k]!=form) ++k;
    if (k==concept_nodes) return false;
    Word old=c.referents[k],cue=!old ? 0 : old==visible ? 1 : (old&visible) ? 2 : 3;
    Word op=(c.genes.expression>>(8+2*cue))&3;
    Word next=op==0 ? old : op==1 ? visible : op==2 ? (old ? old&visible : visible) : old|visible;
    if (next || c.nouns[k]) {c.nouns[k]=form;c.referents[k]=next;}
    return next!=0;
}
AX_FN Word valid(const RouteWord &w,Word block) {
    // Lanes 0..15 and 16..31 test the same first synapse with different
    // second synapses. Fifteen in the second position terminates the route.
    Word a=2*block,b=a+1,first=w.inputs&0x7fffu;
    return ((w.inputs&(1u<<a)) ? first : 0) | ((b==15 || (w.inputs&(1u<<b))) ? first<<16 : 0);
}
AX_FN Word selection(const RouteWord &w,Word k,Word level) {
    Word a=w.low[k],b=w.high[k];
    return valid(w,k)&(level==3 ? a&b : level==2 ? b : level==1 ? a|b : ~0u);
}
AX_FN void consolidate(Concepts &c,Word start) {
    for (Word j=start;j<concept_words && c.words[j].form;++j) {
        auto &w=c.words[j];w.walk=0;Word peak=3;
        for (;peak>0;--peak) {Word any=0;for (Word k=0;k<8;++k) any|=selection(w,k,peak);if (any) break;}
        Word floor=c.genes.expression&3,slack=(c.genes.expression>>2)&3;
        w.threshold=peak<floor ? 4 : (peak>slack+floor ? peak-slack : floor);
        if (w.threshold>3) continue;
        Count common=0;bool disputed=false;
        for (Word k=0;k<8 && !disputed;++k) {
            Word mask=selection(w,k,w.threshold);
            while (mask) {
                Word bit=Word(__builtin_ctz(mask));mask&=mask-1;Count route=wiring(c,32*k+bit);
                if (!route || (common && common!=route)) {disputed=true;break;}
                common=route;
            }
        }
        if (!disputed) w.walk=common;
    }
}
AX_FN Word evidence(Activation v,Word answer) {
    Word bit=1u<<answer;
    if (!v.gap && v.cells==bit) return 1;
    if (!(v.cells&bit)) return 2;
    return v.gap ? 0 : 3;
}
AX_FN Word update(Word program,Word old,Word cue) {return (program>>(2*(4*cue+old)))&3;}
AX_FN void use(RouteWord &w) {if ((w.inputs>>16)<65535) w.inputs+=1u<<16;}
AX_FN bool depends_on(const RouteWord &w,Word atom) {
    if (!(w.inputs&(1u<<atom)) || w.threshold>3) return false;
    Word first=(1u<<atom)|(1u<<(16+atom));
    for (Word k=0;k<8;++k) {
        Word picked=selection(w,k,w.threshold);
        if ((picked&first) || (k==atom/2 && (picked&(0xffffu<<(16*(atom&1)))))) return true;
    }
    return false;
}
AX_FN Word recycle(Concepts &c) {
    Word policy=(c.genes.expression>>24)&3;if (!policy || c.pending.form) return concept_words;
    Word victim=concept_words,minimum=~0u;
    for (Word k=0;k<concept_words;++k) {
        Word cost=policy==1 ? c.words[k].inputs>>16 : policy==2 ? concept_words-k : k;
        for (Word j=k+1;j<concept_words;++j) if (depends_on(c.words[j],8+k)) cost+=1u<<16;
        if (cost<=minimum) {minimum=cost;victim=k;}
    }
    // A fresh acoustic form must never silently change the inputs of an old
    // relation. Remove retired contacts before reusing this physical slot.
    for (Word j=victim+1;j<concept_words;++j) {
        auto &w=c.words[j];w.inputs&=~(1u<<(8+victim));
        for (Word k=0;k<8;++k) {Word keep=valid(w,k);w.low[k]&=keep;w.high[k]&=keep;}
    }
    c.words[victim]={};return victim;
}
AX_FN Word recruit(Concepts &c,Count form,bool reuse) {
    if (!form) return concept_words;
    Word j=word(c,form,true);if (j>=concept_words) j=recycle(c);if (j>=concept_words) return j;
    auto &w=c.words[j];
    if (!w.form) {
        w.form=form;w.inputs=255;
        if (reuse) for (Word i=0;i<j;++i) if (c.words[i].walk) w.inputs|=1u<<(8+i);
        Word initial=(c.genes.expression>>4)&3;
        for (Word k=0;k<8;++k) {w.low[k]=(initial&1) ? valid(w,k) : 0;w.high[k]=(initial&2) ? valid(w,k) : 0;}
    } else if (reuse && !w.walk && (c.genes.expression&(1u<<23))) {
        Word available=0;for (Word i=0;i<j;++i) if (c.words[i].walk) available|=1u<<(8+i);
        if (available&~w.inputs) {
            RouteWord before{};before.inputs=w.inputs;w.inputs|=available;Word initial=(c.genes.expression>>4)&3;
            for (Word k=0;k<8;++k) {
                Word fresh=valid(w,k)&~valid(before,k);
                if (initial&1) w.low[k]|=fresh;
                if (initial&2) w.high[k]|=fresh;
            }
        }
    }
    use(w);return j;
}
AX_FN bool demonstrate(Concepts &c,Count form,Word source,Word answer,bool reuse=true) {
    if (source>=32 || answer>=32 || !(c.domain&(1u<<source)) || !(c.domain&(1u<<answer))) return false;
    Word j=recruit(c,form,reuse);if (j>=concept_words) return false;auto &w=c.words[j];
    Activation first[15]{};
    for (Word i=0;i<15;++i) if (w.inputs&(1u<<i)) first[i]=propagate(c.views[0],c.domain,atom(c,i),{1u<<source,0});
    for (Word k=0;k<8;++k) {
        Word candidates=valid(w,k),a=0,b=0;
        while (candidates) {
            Word lane=Word(__builtin_ctz(candidates)),mask=1u<<lane,id=32*k+lane;candidates&=candidates-1;
            auto result=first[id&15];Word second=id>>4;
            if (second!=15) result=propagate(c.views[0],c.domain,atom(c,second),result);
            Word old=Word(bool(w.low[k]&mask))+2*Word(bool(w.high[k]&mask));
            Word next=update(c.genes.high,old,evidence(result,answer));
            if (next&1) a|=mask;
            if (next&2) b|=mask;
        }
        w.low[k]=a;w.high[k]=b;
    }
    consolidate(c,j);return true;
}
AX_FN Word probe(const Concepts &c,Word j,Word offered,Word random,bool indiscriminate) {
    if (j>=concept_words) return hidden_link;
    const auto &w=c.words[j];offered&=c.domain;
    if (!offered || (w.walk && !indiscriminate)) return hidden_link;
    Word count=Word(__builtin_popcount(offered)),order=random%count,cycle=offered;
    for (Word i=0;i<order;++i) cycle&=cycle-1;
    Word first=Word(__builtin_ctz(cycle)),chosen=first,best=0,drive=(c.genes.expression>>20)&3;
    if (indiscriminate || !drive) return chosen;
    if (w.threshold>3) return first;
    for (Word i=0;i<32;++i) {
        Word x=(first+i)&31;if (!(offered&(1u<<x))) continue;
        unsigned short frequencies[32]{};Word n=0,pairs=0,fired=0,peak=0;
        for (Word k=0;k<8;++k) {
            Word mask=selection(w,k,w.threshold);
            while (mask) {
                Word bit=Word(__builtin_ctz(mask)),code=32*k+bit;mask&=mask-1;
                auto a=propagate(c.views[0],c.domain,atom(c,code&15),{1u<<x,0});
                if ((code>>4)!=15) a=propagate(c.views[0],c.domain,atom(c,code>>4),a);
                Word y=identifier(a);if (y>=32) continue;
                pairs+=n-frequencies[y];++n;Word f=++frequencies[y];if (f>peak) peak=f;fired|=1u<<y;
            }
        }
        Word score=drive==1 ? Word(__builtin_popcount(fired))-Word(fired!=0) : drive==2 ? pairs : n-peak;
        if (score>best) {best=score;chosen=x;}
    }
    return best ? chosen : hidden_link;
}
AX_FN bool ask(Concepts &c,Count form,Word offered,bool random) {
    if (c.pending.form) return false;
    Word j=word(c,form);Word x=probe(c,j,offered,mix(Word(form)^Word(c.questions)^offered),random);
    if (x>=32) return false;
    c.pending.form=form;c.pending.source=x;c.pending.domain=c.domain;c.pending.scene=c.views[0];
    ++c.questions;
    for (Word k=0;k<7;++k) c.pending.prior[k]=c.words[k].walk;
    return true;
}
AX_FN bool reply(Concepts &c,Word answer) {
    if (!c.pending.form || answer>=32 || !(c.pending.domain&(1u<<answer))) return false;
    // Restore only the evidence available at the question. The current world
    // and its compiled meanings must survive a delayed answer.
    auto live=c.views[0];Word domain_now=c.domain;
    Count current[7];for (Word j=0;j<7;++j) {current[j]=c.words[j].walk;c.words[j].walk=c.pending.prior[j];}
    c.views[0]=c.pending.scene;c.domain=c.pending.domain;
    bool accepted=demonstrate(c,c.pending.form,c.pending.source,answer);
    c.views[0]=live;c.domain=domain_now;
    for (Word j=0;j<7;++j) c.words[j].walk=current[j];
    consolidate(c,word(c,c.pending.form));c.pending={};
    return accepted;
}
AX_FN Activation infer_view(const Concepts &c,Word j,const RelationalView &v,Word domain,Activation initial) {
    const auto &w=c.words[j];
    if (w.walk) return propagate(v,domain,w.walk,initial);
    if (w.threshold>3) return uncertain(domain);
    Activation out{0,0};bool fired=false;
    for (Word k=0;k<8;++k) {
        Word mask=selection(w,k,w.threshold);
        while (mask) {
            Word bit=Word(__builtin_ctz(mask));mask&=mask-1;fired=true;
            Word code=32*k+bit;auto p=propagate(v,domain,atom(c,code&15),initial);
            if ((code>>4)!=15) p=propagate(v,domain,atom(c,code>>4),p);
            out.cells|=p.cells;out.gap|=p.gap;
            if (out.gap && out.cells==domain) return out;
        }
    }
    return fired ? out : uncertain(domain);
}
AX_FN Activation infer(const Concepts &c,Word j,Word bank,Activation initial) {
    return infer_view(c,j,c.views[bank],c.domain,initial);
}
AX_FN Word scope(const Concepts &c,Count form,bool acquire=false) {
    if (form) for (Word j=0;j<concept_scopes;++j) {
        if (c.scopes[j].form==form) return j;
        if (!c.scopes[j].form) return acquire ? j : concept_scopes;
    }
    return concept_scopes;
}
AX_FN Word views(const Concepts &c,Word k) {
    if (k==concept_scopes) return 1;
    Word s=c.scopes[k].strength,a=s&3,b=(s>>2)&3,minimum=c.genes.expression&3,slack=(c.genes.expression>>2)&3;
    return Word(a>=minimum && a+slack>=b)|(Word(b>=minimum && b+slack>=a)<<1);
}
AX_FN bool point_view(Concepts &c,Count form,Word j,Word source,Word answer) {
    Word k=scope(c,form,true);
    if (k>=concept_scopes || j>=concept_words || source>=32 || answer>=32 || !(c.domain&(1u<<source)) || !(c.domain&(1u<<answer))) return false;
    auto &s=c.scopes[k];if (!s.form) {s.form=form;s.strength=5*((c.genes.expression>>4)&3);}
    Word strengths=0;
    for (Word b=0;b<2;++b) {
        auto v=infer(c,j,b,{1u<<source,0});
        strengths|=update(c.genes.high,(s.strength>>(2*b))&3,evidence(v,answer))<<(2*b);
    }
    s.strength=strengths;++s.observations;return true;
}
AX_FN ConceptReply converse(Concepts &c,const Count *tokens,Word length,bool statement,bool shared) {
    ConceptReply out{};Word names[2]{},count=0,context=concept_scopes,first=0,steps=0;
    // Two acquired names can ground the intervening unfamiliar sound in an
    // observed scene. A consolidated primitive can instead convey a new fact.
    if (statement && length==3 && ((c.genes.expression>>22)&1)) {
        Word a=noun(c,tokens[0]),b=noun(c,tokens[2]),op=word(c,tokens[1]);
        Count path=op<concept_words ? c.words[op].walk : 0;
        if (unique(a) && unique(b) && (path<8 || path>=16) && noun_slot(c,tokens[1])==concept_nodes && scope(c,tokens[1])==concept_scopes &&
            demonstrate(c,tokens[1],Word(__builtin_ctz(a)),Word(__builtin_ctz(b)))) {
            out.handled=4;out.start=a;out.cells=b;out.context=1;return out;
        }
    }
    for (Word i=0;i<length;++i) {
        Count s=tokens[i];Word op=word(c,s),observer=scope(c,s),tag=noun_slot(c,s);
        if (op<concept_words) {if (!steps) first=op;++steps;}
        else if (observer<concept_scopes) context=observer;
        else if (tag<concept_nodes) {if (count<2) names[count]=c.referents[tag]&c.domain;++count;}
    }
    if (!steps) {
        if (statement && c.pending.form && count==1 && unique(names[0]) && reply(c,Word(__builtin_ctz(names[0])))) out.handled=3;
        return out;
    }
    for (Word i=0;i<length;++i) {Word op=word(c,tokens[i]);if (op<concept_words) use(c.words[op]);}
    out.handled=1;out.steps=steps;out.start=names[0];out.context=views(c,context);
    if (!count || count>2 || !out.context || !names[0] || (count==2 && !names[1])) {out.gap=1;out.sound=c.verdicts[2];return out;}
    if (statement) {
        Count walk=c.words[first].walk;
        if (count==2 && unique(names[0]) && unique(names[1]) && steps==1 && walk>=8 && walk<16 && context==concept_scopes) {
            Word a=Word(__builtin_ctz(names[0])),b=Word(__builtin_ctz(names[1])),raw=Word(walk&7);
            observe(c,raw&3,raw<4 ? a : b,raw<4 ? b : a,shared,true);
            out.handled=2;
        } else {out.gap=1;out.sound=c.verdicts[2];}
        return out;
    }
    for (Word bank=0;bank<2;++bank) if (out.context&(1u<<bank)) {
        Activation pulse{names[0],0};
        bool trace=unique(out.context);out.trace[0]=names[0];
        for (Word j=0;j<length;++j) {
            Word op=word(c,tokens[j]);if (op>=concept_words) continue;
            Count walk=c.words[op].walk;
            if (!trace || !walk) {pulse=infer(c,op,bank,pulse);trace=false;continue;}
            while (walk>1) {
                pulse=step(c.views[bank],c.domain,Word(walk)&7,pulse);walk>>=3;
                if (++out.hops<=8) out.trace[out.hops]=pulse.cells;
            }
        }
        out.cells|=pulse.cells;out.gap|=pulse.gap;
    }
    if (count==2) {
        Word answer=!(out.cells&names[1]) ? 0 : !out.gap && unique(out.cells) && out.cells==names[1] ? 1 : 2;
        out.sound=c.verdicts[answer];
    } else {
        out.sound=pronounce(c,{out.cells,out.gap});
        if (!out.sound) {
            out.sound=c.verdicts[2];
            if (steps==1 && out.context==1 && !c.words[first].walk && ((c.genes.expression>>20)&3) &&
                ask(c,c.words[first].form,c.domain,false)) {
                out.subject=pronounce(c,{1u<<c.pending.source,0});
                if (out.subject) out.question=c.pending.form;else c.pending={};
            }
        }
    }
    return out;
}
}
