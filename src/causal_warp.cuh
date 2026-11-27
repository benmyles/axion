#pragma once
#include "causal.cuh"
#include "concept_warp.cuh"

namespace axion::life::causal {
AX_FN Count voted(bool low,bool high) {Word a=meaning::ballot(low),b=meaning::ballot(high);return a|(Count(b)<<32);}
AX_FN Count at_lane(Count v,Word lane) {
    Word a=meaning::broadcast(Word(v),lane),b=meaning::broadcast(Word(v>>32),lane);return a|(Count(b)<<32);
}
struct Decision {Word action=no_operation,distance=0,solved=0,impossible=0;};
// One warp is one mind. Each lane owns two possible local configurations.
// The inherited recruitment gate compares the possible outcomes of each
// motor transition with the preceding goal wave.
AX_FN Decision seek(const CausalModel &m,Word start,Word require,Word value,Word enabled,Word lane,
                    Intention *trace=nullptr,Word object=32,bool episodic=false,Word limit=64,Count desired=0,Count possible_goal=0,Branch *branch=nullptr) {
    if (!lane && trace) {trace->object=object;trace->start=start;trace->require=require;trace->value=value;trace->length=0;trace->branch=0;}
    if (!lane && branch) *branch={};
    Count goal=desired ? desired : matches(require,value),reached=goal;if (reached&(1ull<<start)) return {no_operation,0,1};
    Count edges[2][operations];
    for (Word h=0;h<2;++h) for (Word a=0;a<operations;++a)
        edges[h][a]=successors(predict(m,a,lane+32*h,episodic));
    Word choice[2]{no_operation,no_operation},depth=0,rotation=(m.genes.expression>>8)%operations;
    Word horizon=1+(m.genes.expression&63);if (limit<horizon) horizon=limit;
    while (depth<horizon) {
        bool recruited[2]{};
        for (Word h=0;h<2;++h) if (!(reached&(1ull<<(lane+32*h)))) {
            for (Word j=0;j<operations;++j) {
                Word a=(rotation+j)%operations;
                if ((enabled&(1u<<a)) && recruit_goal(m.genes,edges[h][a],reached)) {choice[h]=a;recruited[h]=true;break;}
            }
        }
        Count fresh=voted(recruited[0],recruited[1]);if (!fresh) break;
        reached|=fresh;++depth;if (fresh&(1ull<<start)) break;
    }
    if (!(reached&(1ull<<start))) {
        Count optimistic=possible_goal ? possible_goal : goal;bool closed=false;
        for (Word beat=0;beat<horizon;++beat) {
            bool possible[2]{};
            for (Word h=0;h<2;++h) for (Word a=0;a<operations;++a)
                possible[h]|=(enabled&(1u<<a)) && (edges[h][a]&optimistic);
            Count next=optimistic|voted(possible[0],possible[1]);if (next==optimistic) {closed=true;break;}
            optimistic=next;if (optimistic&(1ull<<start)) break;
        }
        if (!lane && trace) trace->branch=closed ? 2 : 3;
        return {no_operation,0,0,Word(closed)};
    }
    Word first=meaning::broadcast(choice[start>>5],start&31);
    if (trace) {
        Word state=start;
        for (Word t=0;t<depth;++t) {
            Word a=meaning::broadcast(choice[state>>5],state&31);
            if (a>=operations) {if (!lane) trace->branch=3;break;}
            Count next=at_lane(edges[state>>5][a],state&31);
            if (!lane) {trace->actions[t]=static_cast<unsigned char>(a);trace->length=t+1;}
            if (!(next&~goal)) break;
            if (next&(next-1)) {
                if (!lane) trace->branch=1;
                if (branch) {
                    Count operation[operations+1];
                    for (Word k=0;k<=operations;++k) operation[k]=next&voted(choice[0]==k,choice[1]==k);
                    for (Word sense=0;sense<traits;++sense) {
                        Count high=plane(sense);Word left=0,right=0;
                        for (Word k=0;k<=operations;++k) {left|=Word(bool(operation[k]&~high))<<k;right|=Word(bool(operation[k]&high))<<k;}
                        if (left!=right && meaning::unique(left) && meaning::unique(right)) {
                            if (!lane) *branch={sense,Word(__builtin_ctz(left)),Word(__builtin_ctz(right))};break;
                        }
                    }
                }
                break;
            }
            state=Word(__builtin_ctzll(next));
        }
        meaning::warp_fence();
    }
    return {first,depth,1};
}
AX_FN Count desired_food(const Agency &a,Word lane) {
    return voted(read_field(a.appetite.field,a.model.genes,lane,patterns(lane))==2,
                 read_field(a.appetite.field,a.model.genes,lane+32,patterns(lane+32))==2);
}
AX_FN void taste(Agency &a,Word state,Word sweet) {
    learn_field(a.appetite.field,a.model.genes,patterns(state),all_pairs,sweet,true);
    ++a.appetite.bites;a.appetite.sampled|=1ull<<state;
}
// Three planes retain the first motor for every possible configuration.
// Code six marks arrival and seven marks an unresolved or unreachable source.
AX_FN void compile_policy(const CausalModel &model,Count goal,Word enabled,Word mode,Word lane,Count *planes) {
    Count reached=goal;Word chosen[2]{7,7};
    for (Word h=0;h<2;++h) if (goal&(1ull<<(lane+32*h))) chosen[h]=6;
    if (goal && mode!=3) {
        Count edges[2][operations];
        for (Word h=0;h<2;++h) for (Word k=0;k<operations;++k) edges[h][k]=successors(predict(model,k,lane+32*h,mode==1));
        Word rotate=(model.genes.expression>>8)%operations,limit=mode==2 ? 1 : 1+(model.genes.expression&63);
        for (Word t=0;t<limit;++t) {
            bool fresh[2]{};
            for (Word h=0;h<2;++h) if (chosen[h]==7) {
                for (Word j=0;j<operations;++j) {
                    Word op=(rotate+j)%operations;
                    if ((enabled&(1u<<op)) && recruit_goal(model.genes,edges[h][op],reached)) {chosen[h]=op;fresh[h]=true;break;}
                }
            }
            Count added=voted(fresh[0],fresh[1]);if (!added) break;reached|=added;
        }
    }
    for (Word k=0;k<3;++k) {Count value=voted(chosen[0]&(1u<<k),chosen[1]&(1u<<k));if (!lane) planes[k]=value;}
}
AX_FN Count enact(const CausalModel &m,Word action,Count possible,Word lane) {
    Count local=0;
    for (Word j=0;j<2;++j) {
        Word x=lane+32*j;if (possible&(1ull<<x)) local|=successors(predict(m,action,x));
    }
    for (Word shift=16;shift;shift>>=1) local|=at_lane(local,lane^shift);return local;
}
struct Reason {Word supported=0,essential=0,repairs=0,values=0,original=0,omitted=0,lost=0;};
AX_FN Reason omission(const CausalModel &m,const Intention &plan,Word cut,Word lane,Count goal=0) {
    if (cut>=plan.length || plan.branch || (!plan.require && !goal)) return {};
    Word prior=plan.start;
    for (Word i=0;i<cut;++i) {
        auto predicted=predict(m,plan.actions[i],prior);if (predicted.unknown) return {};prior=predicted.on;
    }
    auto step=predict(m,plan.actions[cut],prior);if (step.unknown) return {};
    if (!goal) goal=matches(plan.require,plan.value);Count sufficient=goal;
    for (Word i=plan.length;i>cut+1;--i) {
        Word a=plan.actions[i-1];bool safe[2];
        for (Word h=0;h<2;++h) safe[h]=!(successors(predict(m,a,lane+32*h))&~sufficient);
        sufficient=voted(safe[0],safe[1]);
    }
    if (!(sufficient&(1ull<<step.on))) return {};
    if (sufficient&(1ull<<prior)) return {1,0,0,0,step.on,prior};
    Count endings=1ull<<prior;
    for (Word i=cut+1;i<plan.length;++i) endings=enact(m,plan.actions[i],endings,lane);
    if (endings&goal) return {};
    Word lost=0;
    for (Word j=0;j<traits;++j) if ((plan.require&(1u<<j)) && !(endings&matches(1u<<j,plan.value))) lost|=1u<<j;
    Word changes=prior^step.on,best=~0u;
    for (Word h=0;h<2;++h) {
        Word subset=lane+32*h;
        if (!(subset&~changes) && (sufficient&(1ull<<(prior^subset)))) {
            Word rank=Word(__builtin_popcount(subset))*64+subset;if (rank<best) best=rank;
        }
    }
    for (Word shift=16;shift;shift>>=1) {Word v=meaning::broadcast(best,lane^shift);if (v<best) best=v;}
    return {1,1,best&63,step.on&(best&63),step.on,prior,lost};
}
}
