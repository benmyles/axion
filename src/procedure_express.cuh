#pragma once
#include "procedure_recur.cuh"

namespace axion::life::procedure {
struct Derivation {Word cost=~0u;Half rule=0;Half start[2]{},length[2]{};};
struct Description {Word status=unrecognized,length=0,shortest=0,intervened=0;};
AX_FN Derivation &parse_at(Derivation *chart,Word stride,Word lo,Word n) {return chart[(n-1)*stride+lo];}
AX_FN bool unwind(const Recursion &p,Slice y,Byte *phrase,Word cap,Word &size) {
    size=p.head+p.tail;if (size>cap) return false;
    if (phrase) for (Word i=0;i<p.head;++i) phrase[i]=p.prefix[i];
    Word lo=0,hi=y.n;
    while (!same(part(y,lo,hi-lo),{p.base,p.center})) {
        Word hit=p.n,step=0,left=0;
        for (Word j=0;j<p.n;++j) {
            auto u=p.arm[j];if (u.cuts&(u.cuts-1)) continue;
            Word n=Word(u.m)-p.center,k=__builtin_ctz(u.cuts);if (!n || n>hi-lo) continue;
            const auto *act=p.wire+u.at+u.n;Word tail=n-k;
            if (!at(y,lo,{act,k}) || !at(y,hi-tail,{act+k+p.center,tail})) continue;
            if (hit!=p.n) return false;
            hit=j;step=n;left=k;
        }
        if (hit==p.n) return false;
        Slice word=token(p,hit);if (size+word.n>cap) return false;
        if (phrase) for (Word i=0;i<word.n;++i) phrase[size-p.tail+i]=word.p[i];
        size+=word.n;lo+=left;hi-=step-left;
    }
    if (phrase) for (Word i=0;i<p.tail;++i) phrase[size-p.tail+i]=p.suffix[i];
    return true;
}
AX_FN Derivation infer_inverse(const Mind &m,Slice full,Word start,Word n,Derivation *chart) {
    auto &p=m.circuit;auto output_span=part(full,start,n);Derivation best;
    for (Word r=0;r<p.count;++r) {
        auto rule=p.cell[r];int branch=switch_to(p,r,m.retina,m.observing);if (branch<0) continue;
        Slice plan=part_action(p,r,branch>0);Word uses[2]{},fixed=0;
        for (Word k=0;k<plan.n;++k) {if (var(plan.p[k])) ++uses[slot(plan.p[k])];else ++fixed;}
        if (!rule.arity) {
            if (rule.input<best.cost && same(output_span,plan)) {best={};best.cost=rule.input;best.rule=static_cast<Half>(r);}
            continue;
        }
        // An unexpressed argument cannot be reconstructed from the motor sequence.
        if (!uses[0] || (rule.arity==2 && !uses[1]) || fixed+uses[0]+uses[1]>n) continue;
        Word budget=n-fixed;
        for (Word a=rule.arity==1 ? budget/uses[0] : 1;a*uses[0]<=budget;++a) {
            Word rest=budget-uses[0]*a,b=uses[1] ? rest/uses[1] : 0;
            if ((uses[1] && (!b || b*uses[1]!=rest)) || (!uses[1] && rest) || !a) continue;
            Derivation trial;trial.rule=static_cast<Half>(r);trial.cost=rule.input-rule.arity;Word position=0,seen=0;
            trial.length[0]=static_cast<Half>(a);trial.length[1]=static_cast<Half>(b);bool possible=true;
            for (Word k=0;k<plan.n && possible;++k) {
                auto op=plan.p[k];
                if (!var(op)) {possible=output_span.p[position++]==op;continue;}
                Word child=slot(op),length=trial.length[child];
                if (!(seen&(1u<<child))) {
                    seen|=1u<<child;trial.start[child]=static_cast<Half>(start+position);
                    if (rule.raw&(1u<<child)) trial.cost+=length;
                    else {
                        if (length>=n) {possible=false;break;}
                        Word contribution=parse_at(chart,full.n,start+position,length).cost;
                        if (contribution==~0u) {possible=false;break;}trial.cost+=contribution;
                    }
                } else possible=same(part(full,trial.start[child],length),part(output_span,position,length));
                position+=length;
            }
            if (possible && trial.cost<best.cost) best=trial;
        }
    }
    for (Word k=0;k<m.recursion.n;++k) {Word cost=0;if (unwind(m.recursion.state[k],output_span,nullptr,best.cost,cost) && cost<best.cost) {
        best={};best.rule=static_cast<Half>(productions+k);best.cost=cost;
    }}
    return best;
}
AX_FN Description articulate(const Mind &m,Slice goal,Derivation *chart,Byte *text,Word capacity) {
    auto root=parse_at(chart,goal.n,0,goal.n);Description out;
    if (root.cost==~0u) return out;
    out.shortest=root.cost;
    if (root.cost>capacity) {out.status=procedure::capacity;return out;}
    struct Call {Half start,n,pc;};Call stack[stack_limit];Word depth=1;stack[0]={0,static_cast<Half>(goal.n),0};
    while (depth) {
        auto &f=stack[depth-1];auto d=parse_at(chart,goal.n,f.start,f.n);
        if (d.rule>=productions) {
            Word len=0;
            if (!unwind(m.recursion.state[d.rule-productions],part(goal,f.start,f.n),text+out.length,capacity-out.length,len)) return {};
            out.length+=len;--depth;continue;
        }
        auto pattern=cue(m.circuit,d.rule);
        if (f.pc==pattern.n) {--depth;continue;}
        Byte a=pattern.p[f.pc++];if (!var(a)) {text[out.length++]=a;continue;}
        Word k=slot(a),n=d.length[k];
        if (m.circuit.cell[d.rule].raw&(1u<<k)) for (Word j=0;j<n;++j) text[out.length++]=goal.p[d.start[k]+j];
        else {
            if (depth==stack_limit) {out.status=procedure::capacity;return out;}
            stack[depth++]={d.start[k],d.length[k],0};
        }
    }
    out.status=recognized;return out;
}
struct Invention {Byte sign[8]{},movement[16]{};Word length=0,actions=0,exposures=0,saving=0;};
AX_FN Word motif_count(const Memory &book,Slice candidate) {
    Word total=0;
    for (Word j=0;j<book.count;++j) {
        auto y=after(book,j);for (Word k=0;k+candidate.n<=y.n;) {
            if (at(y,k,candidate)) {++total;k+=candidate.n;}else ++k;
        }
    }
    return total;
}
AX_FN bool affordable(const Mind &m,Slice word) {
    for (Word i=0;i<m.circuit.count;++i) {
        auto x=cue(m.circuit,i);Word head=0;while (head<x.n && !var(x.p[head])) ++head;
        Word common=word.n<head ? word.n : head;if (common && same(part(word,0,common),part(x,0,common))) return false;
    }
    for (Word i=0;i<m.recursion.n;++i) {
        auto &r=m.recursion.state[i];Word common=word.n<r.head ? word.n : r.head;
        if (common && same(part(word,0,common),{r.prefix,common})) return false;
    }
    return true;
}
AX_FN Invention innovate(const Mind &m,Word seed,Word length) {
    Invention result;if (!(m.genes.high&16)) return result;
    // Borrow actual sensory sounds for a new vocal form. No meaning or
    // spelling is assigned by the habitat or passed to another animal.
    Word from=0;Byte palette[letters];
    for (Word a=1;a<=m.ear.size;++a) {
        bool heard=false;
        for (Word j=0;j<m.memory.count && !heard;++j) {auto x=before(m.memory,j);for (Word k=0;k<x.n;++k) heard|=x.p[k]==a;}
        if (heard) palette[from++]=static_cast<Byte>(a);
    }
    if (!from) return {};
    for (Word k=0;k<256 && !result.length;++k) {
        Word size=1+((m.genes.expression>>21)&1);
        for (Word j=0;j<size;++j) result.sign[j]=palette[mix(seed^k*5137^j*7919)%from];
        if (affordable(m,{result.sign,size})) result.length=size;
    }
    if (!result.length) return {};
    for (Word i=0;i<m.memory.count;++i) {
        auto trace=after(m.memory,i);
        for (Word n=2;n<=length && n<=trace.n;++n) for (Word pos=0;pos+n<=trace.n;++pos) {
            auto section=part(trace,pos,n);bool already=false;
            for (Word j=0;j<m.circuit.count;++j) if (!m.circuit.cell[j].arity && same(section,output(m.circuit,j))) {already=true;break;}
            if (already) continue;
            Word hits=motif_count(m.memory,section);
            Word saved=hits*(n-1),cost=2+n+result.length;
            if (hits<2+((m.genes.expression>>14)&3) || saved<=cost || saved-cost<=result.saving) continue;
            result.actions=n;result.exposures=hits;result.saving=saved-cost;
            for (Word k=0;k<n;++k) result.movement[k]=section.p[k];
        }
    }
    if (!result.actions) return {};
    return result;
}
}
