#pragma once
#include "procedure_feedback.cuh"
#include "axion/causal.hpp"

namespace axion::life::procedure {
constexpr Byte unknown_state=255;
struct MotorModel {
    Byte actuator[letters+1]{},next[operations][local_states]{};
};
// A column holds the complete denotation of a learned procedure. The
// destination and the pulse count are kept in separate coalesced arrays.
struct Transfer {Count pulses[local_states]{};Byte destination[local_states]{};};
struct Landing {Count pulses=0;Word state=unknown_state;};
AX_FN void contact(const MotorModel &p,Landing &at,Byte ear) {
    Word act=p.actuator[ear];
    if (act>=operations || at.pulses==~0ull) {at.state=unknown_state;return;}
    at.state=p.next[act][at.state];++at.pulses;
}
AX_FN Landing project(const Mind &m,const Decoder &d,const MotorModel &p,const Node &n,const Transfer *rows,Slice input,Word origin) {
    Landing at{0,origin};int arm=switch_to(m.circuit,n.rule,origin,true);
    if (arm<0) return {};
    auto moves=part_action(m.circuit,n.rule,arm!=0);
    for (Word i=0;i<moves.n && at.state<local_states;++i) {
        auto op=moves.p[i];
        if (!var(op)) contact(p,at,op);
        else if (op==call0 || op==call1) {
            const auto &child=rows[op==call0 ? n.left : n.right];
            Count add=child.pulses[at.state];
            if (add>~0ull-at.pulses) return {};
            at.pulses+=add;at.state=child.destination[at.state];
        } else {
            auto raw=bound(input,n,d.shape[n.rule],slot(op));
            for (Word j=0;j<raw.n && at.state<local_states;++j) contact(p,at,raw.p[j]);
        }
    }
    return at;
}
struct Trial {Count movements=0;Word state=0,digest=0;};
struct GardenFocus {Count name=0;Word site=32,eyes=3,enabled=63,execute=0;};
struct GroundReply {GardenReply motion{};Count predicted_moves=0;Word start=64,expected=255,stop=0;};
constexpr Word max_steps=12,palette_limit=operations+guard_limit*operations*(operations-1);
struct Gesture {Byte first=0,second=0,gate=255,cost=0;};
struct Palette {
    Byte literal[operations]{},join=255,reverse=0;Half size=0;
    Gesture choice[palette_limit]{};Byte fate[palette_limit][local_states]{};
};
AX_FN void inventory(const Mind &m,const MotorModel &p,Palette &v,Word senses) {
    v={};for (auto &r:v.literal) r=255;
    const auto &c=m.circuit;
    for (Word i=0;i<c.count;++i) {
        auto r=c.cell[i];auto y=output(c,i);
        if (!r.arity && y.n==1 && p.actuator[y.p[0]]<operations) {
            auto &best=v.literal[p.actuator[y.p[0]]];
            if (best==255 || c.cell[best].input>r.input) best=static_cast<Byte>(i);
        }
        if (r.arity==2 && !r.raw && !(r.flags>>2) && y.n==2 && (y.p[0]==call0 || y.p[0]==call1) &&
            y.p[0]+y.p[1]==call0+call1 && (v.join==255 || r.input<c.cell[v.join].input)) {
            v.join=static_cast<Byte>(i);v.reverse=y.p[0]==call1;
        }
    }
    auto stage=[&](Word a,Word b,Word gate) {
        if (v.size==palette_limit) return;
        auto &step=v.choice[v.size++];
        step={static_cast<Byte>(a),static_cast<Byte>(b),static_cast<Byte>(gate),c.cell[v.literal[a]].input};
        if (gate!=255) step.cost+=c.cell[gate].input-2+c.cell[v.literal[b]].input;
    };
    for (Word a=0;a<operations;++a) if (v.literal[a]!=255) stage(a,a,255);
    if (!senses) return;
    for (Word g=0;g<c.count;++g) {
        if (!(c.cell[g].flags>>2) || c.cell[g].arity!=2 || c.cell[g].raw) continue;
        auto l=part_action(c,g,false),r=part_action(c,g,true);
        if (l.n!=1 || r.n!=1 || (l.p[0]!=call0 && l.p[0]!=call1) || l.p[0]+r.p[0]!=call0+call1) continue;
        for (Word a=0;a<operations;++a) for (Word b=0;b<operations;++b)
            if (a!=b && v.literal[a]!=255 && v.literal[b]!=255) stage(a,b,g);
    }
}
AX_FN Word chosen(const Mind &m,Gesture g,Word retina) {
    if (g.gate==255) return g.first;
    int arm=switch_to(m.circuit,g.gate,retina,true);if (arm<0) return operations;
    return part_action(m.circuit,g.gate,arm!=0).p[0]==call0 ? g.first : g.second;
}
struct InferredPlan {Count sound[performance_span]{};Word n=0;};
AX_FN InferredPlan pronounce(const Mind &m,const Palette &p,const Byte *stage,Word steps) {
    InferredPlan out;
    auto speak=[&](Byte x){if (out.n<performance_span) out.sound[out.n]=m.ear.model[x];++out.n;};
    auto gesture=[&](Word index) {
        const auto &g=p.choice[stage[index]];auto word=cue(m.circuit,p.literal[g.first]);
        if (g.gate==255) {for (Word j=0;j<word.n;++j) speak(word.p[j]);return;}
        auto form=cue(m.circuit,g.gate);
        for (Word j=0;j<form.n;++j) {
            Byte x=form.p[j];if (!var(x)) {speak(x);continue;}
            auto leaf=cue(m.circuit,p.literal[slot(x) ? g.second : g.first]);for (Word t=0;t<leaf.n;++t) speak(leaf.p[t]);
        }
    };
    struct Frame {Byte stage,pc;};Frame stack[max_steps];Word depth=1;stack[0]={0,0};
    while (depth) {
        auto &f=stack[depth-1];if (Word(f.stage)+1==steps) {gesture(f.stage);--depth;continue;}
        auto form=cue(m.circuit,p.join);
        if (f.pc==form.n) {--depth;continue;}
        auto x=form.p[f.pc++];if (!var(x)) speak(x);
        else if (slot(x)==p.reverse) gesture(f.stage);
        else {Word next=f.stage+1;stack[depth++]={static_cast<Byte>(next),0};}
    }
    return out;
}
// A suffix is interchangeable with another suffix on precisely those
// origins whose final state satisfies the target. Store the witness too.
struct Suffix {Count accepts=0;Byte step[max_steps]{};Half cost=0;Byte length=0;};
struct Bid {Count representative=0,rank=0;};
struct BeamStats {Word occupied=0,collision=0,expanded=0;};

}
