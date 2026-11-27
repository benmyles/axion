#include "causal_warp.cuh"
#include "axion/binding.hpp"
#include "causal_world.cuh"
using namespace axion;
using namespace axion::life;
#define KERNEL extern "C" __attribute__((global))
AX_FN Word thread_index() {
    Word i,b,n;asm("mov.u32 %0, %%tid.x;":"=r"(i));asm("mov.u32 %0, %%ctaid.x;":"=r"(b));asm("mov.u32 %0, %%ntid.x;":"=r"(n));return i+b*n;
}
KERNEL void causal_live(const RecollectionGenes *pool,RecallScore *out,Word count,Word worlds,Word seed,Word cycles,Word lessons,
        Word,Word cut,Word first,Word batch,RecollectionGenes,BindingGenes,Word) {
    Word local=thread_index();if (local>=batch) return;Word i=local+first;if (i>=count*worlds) return;
    out[i]=greenhouse::lifetime(pool[i/worlds],mix(seed^(i%worlds)*0x4e686591u),cycles,lessons,cut);
}
KERNEL void causal_merit(const RecallScore *out,Word *fitness,Word count,Word worlds) {
    Word i=thread_index();if (i>=count) return;Word grade=0;
    for (Word j=0;j<worlds;++j) {
        auto &s=out[i*worlds+j];Word parts[]{8*s.one,8*s.faint,s.noise,s.delay,s.revision,8*s.empty},weak=parts[0];
        for (Word v:parts) {grade+=v;if (v<weak) weak=v;}grade+=4*weak;
    }
    fitness[i]=grade;
}
KERNEL void causal_trials(RecollectionGenes genes,CausalModel *models,Intention *plans,CausalAudit *out,Word worlds,Word seed,Word lessons,Word cut) {
    Word t=thread_index(),i=t/32,lane=t&31;if (i>=worlds) return;
    Word key=mix(seed^i*0x7157b23du),negative=mix(key)&63;
    auto world=greenhouse::garden(key);auto &m=models[i];auto &trace=plans[i];
    if (!lane) {m.genes=genes;for (auto &v:m.visited) v=0;}
    meaning::warp_fence();
    for (Word field=lane;field<traits*operations;field+=32) {
        Word a=field/traits,p=field%traits;auto &f=m.fields[a][p];f={};Count seen=0;
        for (Word j=0;j<lessons && cut!=1;++j) {
            Word salt=mix(key+a*0xa51c91bu+j/64),x=greenhouse::permute(j&63,salt);
            Word y=causal::evaluate(world.gates[a][p],cut==2 ? greenhouse::permute((j+11)&63,salt) : x);
            greenhouse::show(f,genes,x,y,p);seen|=1ull<<x;
        }
        if (!p) m.visited[a]=seen;
    }
    meaning::warp_fence();Word require=63,value=63^negative;
    auto decision=causal::seek(m,negative,require,value,63,lane,&trace,0,cut==3,cut==4 ? 1 : 64);
    meaning::warp_fence();
    Word omission=trace.length ? key%trace.length : 0;
    auto reason=causal::omission(m,trace,omission,lane);
    Word disabled=(key>>12)%operations;auto blocked=causal::seek(m,negative,require,value,63^(1u<<disabled),lane,nullptr,0,cut==3,cut==4 ? 1 : 64);
    if (lane) return;
    CausalAudit s{};s.unreachable=blocked.impossible;
    for (Word a=0;a<operations;++a) {
        for (auto &f:m.fields[a]) s.compiled+=f.compiled!=0;
        Word x=greenhouse::permute(63,mix(key+a*0xa51c91bu));
        auto pred=causal::predict(m,a,x,cut==3);s.forecasts+=!pred.unknown && pred.on==causal::physical(world,a,x);s.decided+=!pred.unknown;
    }
    if (decision.solved && !trace.branch) {
        Word actual=negative,omitted=negative,prior=0,changed=0;
        for (Word k=0;k<trace.length;++k) {
            if (k==omission) prior=actual;
            actual=causal::physical(world,trace.actions[k],actual);
            if (k==omission) changed=actual^prior;else omitted=causal::physical(world,trace.actions[k],omitted);
        }
        s.reached=actual==value;s.length=trace.length;s.necessary=s.reached && omitted!=value;
        if (s.necessary && reason.essential) {
            Word best=~0u;
            for (Word subset=changed;;subset=(subset-1)&changed) {
                Word x=prior^subset;for (Word k=omission+1;k<trace.length;++k) x=causal::physical(world,trace.actions[k],x);
                Word rank=Word(__builtin_popcount(subset))*64+subset;if (x==value && rank<best) best=rank;
                if (!subset) break;
            }
            s.reason=(best&63)==reason.repairs && ((prior^changed)&reason.repairs)==reason.values;
        }
    }
    out[i]=s;
}
#include "agency_dialogue.cuh"
#include "orchard.cuh"
KERNEL void causal_census(const CausalModel *m,Count *choices,Word n) {
    Word t=thread_index(),i=t/32;if (i>=n) return;Word code=i%729,require=0,value=0;
    for (Word j=0;j<traits;++j) {Word v=code%3;code/=3;if (v) require|=1u<<j;if (v==2) value|=1u<<j;}
    causal::compile_policy(*m,causal::matches(require,value),i/729,0,t&31,choices+3*i);
}
