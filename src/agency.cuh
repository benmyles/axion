#pragma once
#include "causal_warp.cuh"

namespace axion::life::volition {
constexpr Word motor_mask=63,modes=127u<<18;
AX_FN Word lookup(const GroundedSignals &speech,Count word) {
    for (Word i=0;word && i<32 && speech.forms[i];++i) if (speech.forms[i]==word) return speech.meanings[i];
    return 0;
}
AX_FN Count vocal(const GroundedSignals &speech,Word signal) {
    for (Word i=0;i<32 && speech.forms[i];++i) if (speech.meanings[i]==(1u<<signal)) return speech.forms[i];
    return 0;
}
AX_FN bool ground(GroundedSignals &speech,RecollectionGenes plastic,Count word,Word sensed) {
    if (!word || !sensed) return false;Word j=0;
    while (j<32 && speech.forms[j] && speech.forms[j]!=word) ++j;
    if (j==32) return false;
    Word old=speech.meanings[j],cue=!speech.forms[j] ? 0 : old==sensed ? 1 : (old&sensed) ? 2 : 3;
    Word op=(plastic.expression>>(8+2*cue))&3;
    speech.forms[j]=word;speech.meanings[j]=op==0 ? old : op==1 ? sensed : op==2 ? (cue ? old&sensed : sensed) : (old|sensed);
    return true;
}
AX_FN void look(Agency &mind,const Concepts &names,Word site,Word value,bool eye,bool other) {
    Word cue=Word(eye)+2*Word(other),op=(names.genes.low>>(4*cue))&3,bit=1u<<site;
    if (op==2 || (op==3 && !eye)) mind.observed&=~bit;
    else if (eye && (op==1 || op==3)) {mind.observed|=bit;mind.scenes[site]=static_cast<unsigned char>(value);}
}
AX_FN void emit(GardenReply &out,Count sound) {
    if (sound && out.count<utterance_limit) out.sounds[out.count++]=sound;
}
AX_FN Word literal(Word bit,Word value) {return ((value&(1u<<bit)) ? 6 : 12)+bit;}
AX_FN void properties(GardenReply &out,const GroundedSignals &speech,Word bits,Word value) {
    for (Word j=0;j<traits;++j) if (bits&(1u<<j)) emit(out,vocal(speech,literal(j,value)));
}
struct Slots {Count words[5]{};Word mask=0,values=0;};
AX_FN Word category(const Agency &a,Count form) {
    for (Word i=0;form && i<8;++i) {if (a.aims[i].form==form) return i;if (!a.aims[i].form) break;}return 8;
}
AX_FN Count field_bits(Word table,Count a,Count b) {
    return ((table&1) ? ~(a|b) : 0)|((table&2) ? a&~b : 0)|((table&4) ? b&~a : 0)|((table&8) ? a&b : 0);
}
AX_FN void remember(GoalEvidence &e,RecollectionGenes g,Word state,Word outcome) {
    Count bit=1ull<<state;Word a=(e.low>>state)&1,b=(e.high>>state)&1;
    Word low=(g.low>>(8+4*!outcome+a+2*b))&1,high=(g.low>>(8+4*outcome+b+2*a))&1;
    e.low=(e.low&~bit)|(low ? bit : 0);e.high=(e.high&~bit)|(high ? bit : 0);
}
// A pair of 64-bit evidence planes is sufficient to test any pair of
// earlier settled concepts. Expanding the input vocabulary adds no synapse
// array per candidate. The projected truth sets are invalidated transitively.
AX_FN bool assemble(Agency &a,Word j) {
    auto &e=a.assembly.field[j];auto g=a.model.genes;
    Count off=field_bits(g.high&15,e.low,e.high),on=field_bits(g.high&15,e.high,e.low);
    Count zero=off&~on,one=on&~off;
    Count inputs[14]{};Word n=traits;
    for (Word k=0;k<traits;++k) inputs[k]=causal::plane(k);
    for (Word k=0;k<j;++k) if ((a.assembly.inputs[j]&(1u<<k)) && (a.assembly.active&(1u<<k))) {
        const auto &earlier=a.assembly.field[k];if (earlier.certain==earlier.possible) inputs[n++]=earlier.certain;
    }
    Count positive=0,negative=0;bool viable=false;
    if (!(field_bits((g.high>>4)&15,e.low,e.high)&(e.low|e.high))) {
        for (Word u=0;u<n;++u) for (Word v=u;v<n;++v) {
            Count x=inputs[u],y=inputs[v],regions[]{~(x|y),x&~y,y&~x,x&y},yes=0,no=0;bool allowed=true;
            for (Count region:regions) {
                bool seen_zero=(region&zero)!=0,seen_one=(region&one)!=0;
                if (seen_zero && seen_one && (g.high&128)) {allowed=false;break;}
                if (!seen_zero || seen_one) yes|=region;
                if (!seen_one || seen_zero) no|=region;
            }
            if (allowed) {positive|=yes;negative|=no;viable=true;}
        }
    }
    e.certain=viable ? ~negative : 0;e.possible=viable ? positive : ~0ull;return viable;
}
AX_FN bool exemplify(Agency &a,Count form,Word state,Word outcome,bool reuse) {
    Word i=category(a,form);
    if (i==8) {
        Word rare=0;
        for (Word j=0;j<8;++j) {if (!a.aims[j].form) {i=j;break;}if (a.aims[j].lessons<a.aims[rare].lessons) rare=j;}
        if (i==8) {if (!(a.model.genes.expression&(1u<<19))) return false;i=rare;}
        for (Word j=i+1;j<8;++j) a.assembly.inputs[j]&=static_cast<unsigned char>(~(1u<<i));
        a.aims[i]={};a.aims[i].form=form;a.assembly.field[i]={};
    }
    auto &g=a.aims[i];causal::learn_field(g.field,a.model.genes,causal::patterns(state),all_pairs,outcome,true);++g.lessons;
    a.assembly.active|=1u<<i;a.assembly.inputs[i]=reuse && (a.model.genes.expression&(1u<<22)) ? static_cast<unsigned char>((1u<<i)-1) : 0;
    auto &e=a.assembly.field[i];remember(e,a.model.genes,state,outcome);
    if (!assemble(a,i) && (a.model.genes.high&(1u<<8))) {e={};remember(e,a.model.genes,state,outcome);assemble(a,i);}
    for (Word j=i+1;j<8;++j) if (a.assembly.active&(1u<<j)) assemble(a,j);
    return true;
}
struct Goals {Count definite=0,possible=0;};
AX_FN Goals targets(const Agency &a,Word categories,Word require,Word value,bool hungry,Word lane) {
    Count concrete=causal::matches(require,value);if (!categories && !hungry) return {concrete,concrete};
    bool yes[2]{true,true},maybe[2]{true,true};
    for (Word h=0;h<2;++h) {
        Word x=lane+32*h;auto p=causal::patterns(x);Word choices=categories;
        while (choices) {
            Word j=Word(__builtin_ctz(choices));choices&=choices-1;
            if (a.assembly.active&(1u<<j)) {
                const auto &e=a.assembly.field[j];yes[h]&=bool(e.certain&(1ull<<x));maybe[h]&=bool(e.possible&(1ull<<x));
            } else {
                Word read=causal::read_field(a.aims[j].field,a.model.genes,x,p);yes[h]&=read==2;maybe[h]&=read!=1;
            }
        }
        if (hungry) {Word r=causal::read_field(a.appetite.field,a.model.genes,x,p);yes[h]&=r==2;maybe[h]&=r!=1;}
    }
    return {concrete&causal::voted(yes[0],yes[1]),concrete&causal::voted(maybe[0],maybe[1])};
}
AX_FN Slots branch_roles(const Agency &a,const Concepts &c,Branch branch) {
    Slots s{};if (branch.sense>=traits) return s;
    s.words[0]=vocal(a.speech,12+branch.sense);s.words[2]=vocal(a.speech,6+branch.sense);
    s.words[1]=branch.low<operations ? vocal(a.speech,branch.low) : c.verdicts[1];
    s.words[3]=branch.high<operations ? vocal(a.speech,branch.high) : c.verdicts[1];return s;
}
AX_FN Slots roles(const Agency &a,const Concepts &c,const causal::Reason &r,Word cut) {
    Slots s{};auto &p=a.intention;s.mask=r.repairs;s.values=r.values;
    s.words[0]=p.object<32 ? meaning::pronounce(c,{1u<<p.object,0}) : 0;s.words[1]=vocal(a.speech,p.actions[cut]);
    if (r.repairs) {
        Word j=Word(__builtin_ctz(r.repairs));
        s.words[2]=vocal(a.speech,literal(j,~r.values));s.words[3]=vocal(a.speech,literal(j,r.values));
    }
    Word goal=r.lost ? r.lost : p.require;if (goal) s.words[4]=vocal(a.speech,literal(Word(__builtin_ctz(goal)),p.value));
    else s.words[4]=a.goal_name ? a.goal_name : vocal(a.speech,Word(Disposition::taste));
    return s;
}
AX_FN bool imitate(ExplanationFrame &frame,const Slots &s,const Count *words,Word n,Word plastic) {
    if (!n || n>8) return false;
    bool fresh=!frame.length || frame.length!=n;
    if (frame.length && frame.length!=n && !(plastic&1)) return false;
    auto next=frame;next.length=static_cast<unsigned char>(n);
    for (Word i=0;i<n;++i) {
        Word match=0;for (Word k=0;k<5;++k) if (s.words[k] && s.words[k]==words[i]) match|=1u<<k;
        next.roles[i]=static_cast<unsigned char>(fresh ? match : next.roles[i]&match);
        next.fixed[i]=(fresh || frame.fixed[i]==words[i]) ? words[i] : 0;
        if (!next.roles[i] && !next.fixed[i]) return false;
    }
    frame=next;return true;
}
AX_FN void explain(GardenReply &out,const Agency &a,const Concepts &c,const causal::Reason &r,Word cut) {
    if (!r.supported || !r.essential) {emit(out,c.verdicts[r.supported ? 0 : 2]);return;}
    auto s=roles(a,c,r,cut);out.repairs=r.repairs;out.values=r.values;
    auto &f=a.explanation;
    if (!f.length) {emit(out,s.words[1]);properties(out,a.speech,r.repairs,r.values);return;}
    for (Word i=0;i<f.length;++i) {
        Word roles=f.roles[i];Count word=f.fixed[i];
        if (roles) {
            Count common=0;bool conflict=false;
            for (Word k=0;k<5;++k) if (roles&(1u<<k)) {if (!s.words[k] || (common && common!=s.words[k])) conflict=true;common=s.words[k];}
            if (conflict || (word && word!=common)) {out.count=0;emit(out,c.verdicts[2]);return;}
            word=common;
            if (meaning::unique(roles) && (roles&12)) {properties(out,a.speech,r.repairs,roles==4 ? ~r.values : r.values);continue;}
            if (roles==16 && a.intention.require) {properties(out,a.speech,r.lost ? r.lost : a.intention.require,a.intention.value);continue;}
        }
        if (!word) {out.count=0;emit(out,c.verdicts[2]);return;}emit(out,word);
    }
}
struct Request {
    Word mode=0,object=32,require=0,value=0,actions=0,first=no_operation,ambiguous=0,categories=0,recall=0;
};
AX_FN Request listen(const Agency &a,const Concepts &c,const Count *words,Word n) {
    Request q{};Word object=0,yes=0,no=0,intent=0,view=concept_scopes,paths=0;
    for (Word i=0;i<n;++i) {
        Word heard=lookup(a.speech,words[i]);
        if (!heard) {
            Word goal=category(a,words[i]);if (goal<8) q.categories|=1u<<goal;
            else {
                Word op=meaning::word(c,words[i]),eye=meaning::scope(c,words[i]);
                if (op<concept_words) ++paths;else if (eye<concept_scopes) view=eye;
                else {Word tag=meaning::noun(c,words[i]);if (tag) object|=tag;}
            }
            continue;
        }
        if (!meaning::unique(heard)) {q.ambiguous=1;continue;}
        if (heard&motor_mask) {if (!q.actions) q.first=Word(__builtin_ctz(heard));++q.actions;}
        else {yes|=(heard>>6)&63;no|=(heard>>12)&63;intent|=heard&modes;q.recall|=heard&(1u<<25);}
    }
    if (!intent) return q;
    q.mode=Word(__builtin_ctz(intent));q.require=yes|no;q.value=yes;q.ambiguous|=!meaning::unique(intent) || bool(yes&no);
    if (paths) {
        Word banks=meaning::views(c,view);Activation result{};
        for (Word k=0;k<2;++k) if (banks&(1u<<k)) {
            Activation active{object,0};
            for (Word i=0;i<n;++i) {Word op=meaning::word(c,words[i]);if (op<concept_words) active=meaning::infer(c,op,k,active);}
            result.cells|=active.cells;result.gap|=active.gap;
        }
        q.object=meaning::identifier(result);q.ambiguous|=q.object>=32;
    } else if (object) {q.ambiguous|=!meaning::unique(object);q.object=Word(__builtin_ctz(object));}
    else if (q.actions || n==1 || q.require || q.categories || q.recall) q.object=a.intention.object;
    return q;
}
AX_FN bool label(Agency &a,const Concepts &c,const Count *words,Word n,bool reuse) {
    if (n!=3 || !(a.model.genes.expression&(1u<<19))) return false;
    Word positive=2,object=0;Count form=0;
    for (Word i=0;i<n;++i) {
        Count w=words[i];if (w==c.verdicts[0]) positive=0;else if (w==c.verdicts[1]) positive=1;
        else if (Word tag=meaning::noun(c,w)) object|=tag;
        else if (!lookup(a.speech,w) && meaning::word(c,w)==concept_words && !form) form=w;
        else return false;
    }
    if (positive>1 || !form || !meaning::unique(object) || !(a.observed&object)) return false;
    return exemplify(a,form,a.scenes[Word(__builtin_ctz(object))],positive,reuse);
}
AX_FN bool pronounce_branch(GardenReply &out,const Agency &a,const Concepts &c) {
    if (!a.conditional.length || a.branch.sense>=traits) return false;
    auto roles=branch_roles(a,c,a.branch);Word initial=out.count;
    if (utterance_limit-initial<a.conditional.length) return false;
    for (Word j=0;j<a.conditional.length;++j) {
        auto &f=a.conditional;Count sound=f.fixed[j];Word mask=f.roles[j];
        for (Word k=0;k<4;++k) if (mask&(1u<<k)) {
            if (!roles.words[k] || (sound && sound!=roles.words[k])) {out.count=initial;return false;}sound=roles.words[k];
        }
        if (!sound) {out.count=initial;return false;}emit(out,sound);
    }
    return true;
}
AX_FN bool interpret_branch(const Agency &a,const Concepts &c,const Count *speech,Word n,Branch &out) {
    auto &f=a.conditional;if (!n || n!=f.length) return false;
    Word axis=traits,low=~0u,high=~0u;
    for (Word j=0;j<n;++j) {
        if (f.fixed[j] && f.fixed[j]!=speech[j]) return false;
        Word roles=f.roles[j];if (!roles) {if (!f.fixed[j]) return false;continue;}
        Word sound=lookup(a.speech,speech[j]);
        for (Word k=0;k<4;++k) if (roles&(1u<<k)) {
            if (k==0 || k==2) {
                Word bits=(sound>>(k==0 ? 12 : 6))&63;if (!meaning::unique(bits)) return false;
                Word sense=Word(__builtin_ctz(bits));if (axis!=traits && axis!=sense) return false;axis=sense;
            } else {
                Word motor=speech[j]==c.verdicts[1] ? operations : (meaning::unique(sound) && (sound&motor_mask)) ? Word(__builtin_ctz(sound)) : ~0u;
                if (motor==~0u) return false;if (k==1) low=motor;else high=motor;
            }
        }
    }
    if (axis==traits || low==~0u || high==~0u) return false;out={axis,low,high};return true;
}
AX_FN Word experiment(const Agency &a,Word enabled,Word focus,Word key,bool blind) {
    Word best=0,chosen=~0u,first=key%(32*operations);
    for (Word i=0;i<32*operations;++i) {
        Word spot=(first+i)%(32*operations),action=spot/32,site=spot&31;
        if (!(a.observed&(1u<<site)) || !(enabled&(1u<<action))) continue;
        if (blind || !(a.model.genes.expression&(1u<<18))) return spot;
        Word x=a.scenes[site];auto p=causal::predict(a.model,action,x);
        Word score=Word(__builtin_popcount(p.unknown))+2*Word(__builtin_popcount(p.unknown&focus))
                   +Word(!(a.model.visited[action]&(1ull<<x)));
        if (chosen==~0u || score>best) {best=score;chosen=spot;}
    }
    return chosen;
}
}
