#pragma once
#include "garden_motion.cuh"
#include "lantern.cuh"
KERNEL void agency_encounter(Agency *a,Concepts *c,Agency *b,Concepts *d,Apparatus *w,const Count *words,GardenPort p,GardenReply *answer) {
    Word lane=thread_index();if (lane>=32 || !a || !c) return;auto &out=*answer;
    if (!lane) out={};meaning::warp_fence();
    if (p.event==GardenEvent::branch) {
        if (!lane && a->intention.branch==1 && a->branch.sense<traits) out.handled=volition::imitate(a->conditional,volition::branch_roles(*a,*c,a->branch),words,p.length,a->model.genes.expression);
        return;
    }
    if (p.event==GardenEvent::imitate) {
        auto r=causal::omission(a->model,a->intention,p.a,lane,a->desired);
        if (!lane && r.essential) out.handled=volition::imitate(a->explanation,volition::roles(*a,*c,r,p.a),words,p.length,a->model.genes.expression);
        return;
    }
    if (p.event==GardenEvent::relay) {
        if (lane || !b || !d || p.a>=32 || !(w->present&(1u<<p.a))) return;
        auto &plan=a->intention;out.handled=1;out.subject=p.a;out.before=w->plots[p.a];out.after=out.before;
        if (plan.branch>1 || !plan.length) return;
        if (p.b==1) return;
        for (Word j=0;j<plan.length;++j) volition::emit(out,volition::vocal(a->speech,plan.actions[j]));
        if (plan.branch==1) volition::pronounce_branch(out,*a,*c);
        if (p.b==2) for (Word j=0;j<out.count/2;++j) {auto v=out.sounds[j];out.sounds[j]=out.sounds[out.count-1-j];out.sounds[out.count-1-j]=v;}
        Branch decoded{};Word prefix=out.count,clause=b->conditional.length;
        bool contingent=clause && out.count>=clause && volition::interpret_branch(*b,*d,out.sounds+out.count-clause,clause,decoded);
        if (contingent) prefix-=clause;
        Word j=0;
        for (;j<prefix;++j) {
            Word heard=volition::lookup(b->speech,out.sounds[j]);
            if (!meaning::unique(heard) || !(heard&volition::motor_mask)) break;
            Word action=Word(__builtin_ctz(heard));if (!(p.enabled&(1u<<action))) break;
            volition::move(a,c,b,d,*w,p.a,action,p.eyes,out);
        }
        if (j==prefix && contingent && (p.eyes&2) && (b->observed&(1u<<p.a))) {
            Word motor=(b->scenes[p.a]&(1u<<decoded.sense)) ? decoded.high : decoded.low;
            if (motor<operations && (p.enabled&(1u<<motor))) volition::move(a,c,b,d,*w,p.a,motor,p.eyes,out);
        }
        out.solved=bool(a->desired&(1ull<<out.after));return;
    }
    if (p.event!=GardenEvent::speech) {
        if (lane) return;
        switch (p.event) {
        case GardenEvent::survey:
            for (Word j=0;j<32;++j) if (w->present&(1u<<j)) volition::sight(a,c,b,d,j,w->plots[j],p.eyes);
            break;
        case GardenEvent::lantern:
        case GardenEvent::open:
            if (!p.a || p.a>32) return;
            *w=p.event==GardenEvent::lantern ? lantern::cabinet(p.b,p.a) : greenhouse::garden(p.b,p.a);a->observed=0;a->intention={};if (b) {b->observed=0;b->intention={};}
            for (Word j=0;j<p.a;++j) volition::sight(a,c,b,d,j,w->plots[j],p.eyes);
            break;
        case GardenEvent::change:
            if (p.a>=operations || p.b>=traits || !p.c) return;
            w->gates[p.a][p.b]=static_cast<unsigned short>(p.c);break;
        case GardenEvent::diet:w->food=p.c;break;
        case GardenEvent::taste: {
            if (p.a>=32 || !(w->present&(1u<<p.a))) return;
            Word state=w->plots[p.a],sweet=causal::evaluate(w->food,state);
            if (p.eyes&1) causal::taste(*a,state,sweet);if (b && (p.eyes&2)) causal::taste(*b,state,sweet);
            out.solved=sweet;out.subject=p.a;out.after=state;volition::emit(out,c->verdicts[sweet]);break;
        }
        case GardenEvent::place:
            if (p.a>=32 || !(w->present&(1u<<p.a)) || p.b>63) return;
            w->plots[p.a]=static_cast<unsigned char>(p.b);volition::sight(a,c,b,d,p.a,p.b,p.eyes);out.subject=p.a;out.after=p.b;break;
        case GardenEvent::inspect:
            if (p.a>=32 || !(w->present&(1u<<p.a))) return;
            out.subject=p.a;out.after=w->plots[p.a];break;
        case GardenEvent::transition:
            if (p.a>=operations || p.b>63 || p.c>63) return;
            volition::demonstrate(a,b,{p.b&p.known_before,p.a,p.c&p.known_after,p.known_before,p.known_after},p.eyes);break;
        case GardenEvent::contact:
            if (p.a>=operations || p.b>=32 || !(w->present&(1u<<p.b)) || !(p.enabled&(1u<<p.a))) return;
            volition::move(a,c,b,d,*w,p.b,p.a,p.eyes,out);break;
        case GardenEvent::experience:
            if (!w->present || p.a>256) return;
            for (Word visit=0;visit<p.a;++visit) {
                for (Word motor=0;motor<operations;++motor) {
                    Word x=greenhouse::permute(visit&63,mix(p.b+motor*929+visit/64)),y=causal::physical(*w,motor,x);
                    volition::demonstrate(a,b,{x,motor,y},p.eyes);
                }
                Word x=greenhouse::permute(visit&63,mix(p.b+10007+visit/64)),sweet=causal::evaluate(w->food,x);
                if (p.eyes&1) causal::taste(*a,x,sweet);if (b && (p.eyes&2)) causal::taste(*b,x,sweet);
            }
            break;
        case GardenEvent::probe: {
            Word k=volition::experiment(*a,p.enabled,p.c,p.a,p.b!=0);
            if (k==~0u || !(w->present&(1u<<(k&31)))) return;
            volition::emit(out,volition::vocal(a->speech,k/32));volition::move(a,c,b,d,*w,k&31,k/32,p.eyes,out);break;
        }
        case GardenEvent::vocabulary:
            for (Word j=0;j<p.length;++j) {
                if (p.eyes&1) volition::ground(a->speech,c->genes,words[j],p.a);
                if (b && d && (p.eyes&2)) volition::ground(b->speech,d->genes,words[j],p.a);
            }
            break;
        case GardenEvent::category:
            if (p.length!=1 || p.a>=32 || p.b>1) return;
            if ((p.eyes&1) && (a->observed&(1u<<p.a))) out.handled|=volition::exemplify(*a,words[0],a->scenes[p.a],p.b,p.reuse!=0);
            if (b && (p.eyes&2) && (b->observed&(1u<<p.a))) out.handled|=volition::exemplify(*b,words[0],b->scenes[p.a],p.b,p.reuse!=0);
            return;
        case GardenEvent::contrast: {
            Word difference=p.a^p.c;
            if (!meaning::unique(difference) || p.a>63 || p.c>63 || p.b>6 || p.d>6 || p.b==p.d) return;
            bool high=(p.a&difference)!=0;Branch shown{Word(__builtin_ctz(difference)),high ? p.d : p.b,high ? p.b : p.d};
            if (p.eyes&1) out.handled|=volition::imitate(a->conditional,volition::branch_roles(*a,*c,shown),words,p.length,a->model.genes.expression);
            if (b && d && (p.eyes&2)) out.handled|=volition::imitate(b->conditional,volition::branch_roles(*b,*d,shown),words,p.length,b->model.genes.expression);
            return;
        }
        default:return;
        }
        out.handled=1;return;
    }
    if (p.a) {
        Word taught=0;if (!lane) {taught=volition::label(*a,*c,words,p.length,p.reuse!=0);if (b && d && p.shared) volition::label(*b,*d,words,p.length,p.reuse!=0);}
        if (meaning::broadcast(taught)) {if (!lane) {out.handled=1;out.kind=25;}return;}
    }
    auto q=volition::listen(*a,*c,words,p.length);if (!q.mode) return;
    if (!lane) {out.handled=1;out.kind=q.mode;out.subject=q.object;out.require=q.require;out.value=q.value;}
    if (q.ambiguous || q.object>=32 || !(a->observed&(1u<<q.object))) {if (!lane) volition::emit(out,c->verdicts[2]);return;}
    Word start=a->scenes[q.object];if (!lane) out.before=out.after=start;auto mode=Disposition(q.mode);
    if (mode==Disposition::describe) {if (!lane) volition::properties(out,a->speech,63,start);return;}
    if (mode==Disposition::taste) {
        Word pulse=causal::read_field(a->appetite.field,a->model.genes,start,causal::patterns(start));
        if (!lane) {volition::emit(out,c->verdicts[pulse==1 ? 0 : pulse==2 ? 1 : 2]);out.solved=pulse!=3;}return;
    }
    if (mode==Disposition::why) {
        auto &t=a->intention;
        if (!t.length || t.object!=q.object) {if (!lane) volition::emit(out,c->verdicts[2]);return;}
        Word cut=0;if (q.actions) while (cut<t.length && t.actions[cut]!=q.first) ++cut;
        auto r=causal::omission(a->model,t,cut,lane,a->desired);
        if (!lane) {out.selected=cut<t.length ? t.actions[cut] : no_operation;out.solved=r.supported;out.before=r.omitted;out.after=r.original;volition::explain(out,*a,*c,r,cut);}
        return;
    }
    if (mode==Disposition::forecast) {
        Count possible=1ull<<start;
        for (Word j=0;j<p.length;++j) {
            Word heard=volition::lookup(a->speech,words[j]);if (meaning::unique(heard) && (heard&volition::motor_mask)) possible=causal::enact(a->model,Word(__builtin_ctz(heard)),possible,lane);
        }
        auto goals=volition::targets(*a,q.categories,q.require,q.value,false,lane);
        if (!lane) {
            if (q.require || q.categories) {Word verdict=!(possible&goals.possible) ? 0 : !(possible&~goals.definite) ? 1 : 2;volition::emit(out,c->verdicts[verdict]);out.solved=verdict!=2;}
            else if (possible && !(possible&(possible-1))) {volition::properties(out,a->speech,63,Word(__builtin_ctzll(possible)));out.solved=1;}
            else volition::emit(out,c->verdicts[2]);
        }
        return;
    }
    if (mode==Disposition::act && !q.require && !q.categories && q.actions) {
        if (lane) return;
        if (!(w->present&(1u<<q.object))) {volition::emit(out,c->verdicts[2]);return;}
        for (Word j=0;j<p.length;++j) {
            Word heard=volition::lookup(a->speech,words[j]);if (!(heard&volition::motor_mask) || !meaning::unique(heard)) continue;
            Word motor=Word(__builtin_ctz(heard));if (!(p.enabled&(1u<<motor))) break;
            volition::move(a,c,b,d,*w,q.object,motor,p.eyes,out);volition::emit(out,words[j]);
        }
        return;
    }
    bool hungry=mode==Disposition::forage;
    auto goals=volition::targets(*a,q.categories,q.require,q.value,hungry,lane);
    Count name=q.categories ? a->aims[Word(__builtin_ctz(q.categories))].form : hungry ? volition::vocal(a->speech,Word(Disposition::taste)) : 0;
    if (!q.categories && !q.require && !hungry) {
        if ((!q.recall && p.length!=1) || a->intention.object!=q.object || !a->desired) {if (!lane) volition::emit(out,c->verdicts[2]);return;}
        q.require=a->intention.require;q.value=a->intention.value;goals.definite=goals.possible=a->desired;name=a->goal_name;
    }
    if (!goals.definite) {if (!lane) volition::emit(out,c->verdicts[goals.possible ? 2 : 0]);return;}
    if (!lane) {out.require=q.require;out.value=q.value;a->desired=goals.definite;a->goal_name=name;}
    auto choice=causal::seek(a->model,start,q.require,q.value,p.enabled,lane,&a->intention,q.object,false,64,goals.definite,goals.possible,&a->branch);
    meaning::warp_fence();
    if (!lane) {out.solved=choice.solved;out.branch=a->intention.branch;}
    if (!choice.solved) {if (!lane) volition::emit(out,c->verdicts[choice.impossible ? 0 : 2]);return;}
    if (choice.action==no_operation) {if (!lane) volition::emit(out,c->verdicts[1]);return;}
    if (mode==Disposition::plan || mode==Disposition::forage) {
        if (!lane) {
            Word x=start;out.trajectory[out.samples++]=static_cast<unsigned char>(start);
            for (Word j=0;j<a->intention.length;++j) {
                Word motor=a->intention.actions[j];volition::emit(out,volition::vocal(a->speech,motor));
                auto next=causal::predict(a->model,motor,x);if (!next.unknown) {x=next.on;out.trajectory[out.samples++]=static_cast<unsigned char>(x);}
            }
            if (a->intention.branch==1) volition::pronounce_branch(out,*a,*c);
        }
        return;
    }
    if (!(w->present&(1u<<q.object)) || !(p.eyes&1)) {if (!lane) {out.solved=0;volition::emit(out,c->verdicts[2]);}return;}
    for (Word t=0;t<utterance_limit && choice.solved && choice.action<operations;++t) {
        if (!lane) {
            volition::move(a,c,b,d,*w,q.object,choice.action,p.eyes,out);volition::emit(out,volition::vocal(a->speech,choice.action));
        }
        meaning::warp_fence();
        Word now=a->scenes[q.object];if (goals.definite&(1ull<<now)) break;
        choice=causal::seek(a->model,now,q.require,q.value,p.enabled,lane,nullptr,32,false,64,goals.definite,goals.possible);
    }
    if (!lane) out.solved=bool(goals.definite&(1ull<<out.after));
}
