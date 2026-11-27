#pragma once
#include "axion/commons.hpp"
#include "agency.cuh"
namespace axion::life::orchard {
constexpr Word eat=6,explore=7;
AX_FN Word decode(const CropPolicy &p,Word x) {
    return Word((p.planes[0]>>x)&1)|Word((p.planes[1]>>x)&1)*2|Word((p.planes[2]>>x)&1)*4;
}
AX_FN void policy(const Agency &a,CropPolicy &cache,Word lane,Word mode) {
    causal::compile_policy(a.model,causal::desired_food(a,lane),63,mode,lane,cache.planes);
    if (!lane) {cache.dirty=0;++cache.builds;}
}
AX_FN Word key(Word seed,Word i,Word day) {return mix(seed^i*0x6d2b79f5u^day*0x93ef18e3u);}
AX_FN Word physical_probe(const Agency &a,Word state,Word draw,bool random) {
    Word choice=draw%operations;if (random) return choice;
    Word best=0;
    for (Word j=0;j<operations;++j) {
        Word op=(draw%operations+j)%operations;auto p=causal::predict(a.model,op,state);
        Word novelty=!(a.model.visited[op]&(1ull<<state));
        Word score=Word(__builtin_popcount(p.unknown))+8*novelty;
        if (score>best) {best=score;choice=op;}
    }
    return choice;
}
AX_FN bool stable(const CausalField &before,const CausalField &now) {
    if (before.compiled && before.compiled==now.compiled) return true;
    if (before.compiled!=now.compiled || before.viable!=now.viable) return false;
    for (Word k=0;k<4;++k) if (before.low[k]!=now.low[k] || before.high[k]!=now.high[k]) return false;
    return true;
}
AX_FN bool consequence(Agency &a,Word op,Word x,Word y,Word mode) {
    if (op==eat) {
        auto before=a.appetite.field;causal::taste(a,x,y);return !stable(before,a.appetite.field);
    }
    bool dirty=mode==1 && !(a.model.visited[op]&(1ull<<x));
    a.model.visited[op]|=1ull<<x;auto patches=causal::patterns(x);
    for (Word j=0;j<traits;++j) {
        auto &f=a.model.fields[op][j];auto before=f;
        causal::learn_field(f,a.model.genes,patches,all_pairs,(y>>j)&1,((x^y)>>j)&1);
        dirty|=!stable(before,f);
    }
    return dirty;
}
AX_FN void plant(SeedBed &bed,Word phase,Word i,bool mixed) {
    bed.dormant_meals=0;
    for (Word j=0;j<32;++j) bed.field.plots[j]=static_cast<unsigned char>(mixed ? mix(phase^i*3319^j)&63 : bed.field.dormant);
}
}
KERNEL void garden_seed(const CommonsSettings *q,SeedBed *out) {
    Word i=thread_index();if (i>=q->count/32) return;
    SeedBed bed{};bed.field=greenhouse::garden(mix(q->current_seed^i*0xf17dbd23u));bed.ancestor=i;
    orchard::plant(bed,q->seed,i,q->mixed_plots!=0);out[i]=bed;
}
KERNEL void garden_transplant(const CommonsSettings *q,const SeedBed *donors,Word count,SeedBed *out) {
    Word i=thread_index();if (i>=q->count/32) return;auto bed=donors[i%count];bed.migrated=0;
    orchard::plant(bed,q->seed,i,q->mixed_plots!=0);out[i]=bed;
}
KERNEL void garden_disperse(const CommonsSettings *q,const SeedBed *previous,const Reservoir *water,SeedBed *next,Word season) {
    Word i=thread_index(),n=q->count/32;if (i>=n) return;Word draw=orchard::key(q->seed,i,season),chosen=i;bool seed_available=false;
    if (q->flora) {
        Word candidates[3]{(i+n-1)%n,i,(i+1)%n},weight[3],total=0;
        for (Word k=0;k<3;++k) {weight[k]=q->flora==2 ? 1 : water[candidates[k]].consumed;total+=weight[k];}
        if (total) {
            seed_available=true;
            Word ticket=draw%total;
            for (Word k=0;k<3;++k) {if (ticket<weight[k]) {chosen=candidates[k];break;}ticket-=weight[k];}
        }
    }
    SeedBed bed=previous[chosen];bed.migrated=Word(chosen!=i);
    if (seed_available && q->pollen && mix(draw+239)%q->pollen==0) {
        Word k=mix(draw+337),locus=k%38;
        if (locus==37) bed.field.dormant^=1u<<((k>>8)%traits);
        else {
            Word old=locus==36 ? bed.field.food : bed.field.gates[locus/6][locus%6];
            Word v=old-1,a=v&7,b=(v>>3)&7,lut=v>>6,site=(k>>11)%6;
            if (site==0) a=(a+1+(k>>16)%5)%traits;
            else if (site==1) b=(b+1+(k>>16)%5)%traits;
            else lut^=1u<<(site-2);
            Word changed=causal::gate(a,b,lut);if (locus==36) bed.field.food=changed;else bed.field.gates[locus/6][locus%6]=static_cast<unsigned short>(changed);
        }
        ++bed.edits;
    }
    orchard::plant(bed,q->seed,i,q->mixed_plots!=0);next[i]=bed;
}
KERNEL void garden_rear(const CommonsSettings *q,const SeedBed *beds,Agency *animals,Concepts *names,CropPolicy *policies,const GardenSpeech *voices) {
    Word t=thread_index(),i=t/32,lane=t&31;if (i>=q->count) return;
    const auto &world=beds[i/32].field;auto &a=animals[i];Word key=orchard::key(q->seed,i,0);
    for (Word field=lane;field<traits*operations;field+=32) {
        Word op=field/traits,p=field%traits;auto &f=a.model.fields[op][p];Count seen=0;
        for (Word visit=0;q->credit && visit<q->exposures;++visit) {
            Word x=greenhouse::permute(visit,mix(key+op*883));
            greenhouse::show(f,a.model.genes,x,causal::evaluate(world.gates[op][p],x),p);seen|=1ull<<x;
        }
        if (!p) a.model.visited[op]=seen;
    }
    if (!lane) {
        policies[i]={};meaning::arrange(names[i],32,true);
        if (q->tutoring) {
            for (Word j=0;j<32;++j) meaning::point(names[i],voices->names[j],1u<<j);
            for (Word j=0;j<25;++j) volition::ground(a.speech,names[i].genes,voices->words[j],1u<<j);
            for (Word j=0;j<3;++j) names[i].verdicts[j]=voices->verdicts[j];
        }
        for (Word v=0;q->credit && v<q->exposures;++v) {Word x=greenhouse::permute(v,mix(key+1907));causal::taste(a,x,causal::evaluate(world.food,x));}
    }
}
KERNEL void garden_prepare(const CommonsSettings *q,const Agency *animals,CropPolicy *policy,const Forager *bodies) {
    Word t=thread_index(),i=t/32;if (i>=q->count || !bodies[i].energy || (q->garden_cache && !policy[i].dirty)) return;
    orchard::policy(animals[i],policy[i],t&31,q->garden_model);
}
KERNEL void garden_sense(const CommonsSettings *q,const SeedBed *beds,const Forager *bodies,Encounter *events) {
    Word i=thread_index();if (i>=q->count) return;const auto &a=bodies[i];auto &world=beds[i/32].field;
    Word state=world.plots[i&31],other=world.plots[(i+1)&31];
    Word light=state|(other<<6)|(1u<<(12+a.receipt))|(1u<<(20+Word(a.energy<8)));
    events[i]={light,0x013f0fffu,0,0,a.receipt==1 ? 1u : a.receipt==2 ? 2u : 0u,a.energy ? EventKind::step : EventKind::probe};
}
KERNEL void garden_choose(const CommonsSettings *q,const SeedBed *beds,const Forager *bodies,Agency *animals,const CropPolicy *policy,const Response *muscles,const Bindings *binding,Word *decisions) {
    Word i=thread_index();if (i>=q->count) return;auto &body=bodies[i];if (!body.energy) {decisions[i]=0;return;}
    auto &a=animals[i];auto &w=beds[i/32].field;Word lane=i&31,key=orchard::key(q->seed,i,body.day);
    Word motor=q->soma ? muscles[i].ports : 0,other=(lane+((motor&1) ? 31 : 1))&31;
    Word site=lane,x=w.plots[lane],y=w.plots[other],code=orchard::decode(policy[i],x),peer=orchard::decode(policy[i],y);
    a.scenes[lane]=static_cast<unsigned char>(x);a.scenes[other]=static_cast<unsigned char>(y);a.observed|=(1u<<lane)|(1u<<other);
    if (peer==orchard::eat || (code==orchard::explore && peer!=orchard::explore) || (code==peer && ((key>>7)&1))) {
        site=other;x=y;code=peer;
    }
    bool directed=code<operations;Word action=code;
    if (q->policy==1) {action=orchard::eat;directed=false;}
    else if (q->policy==2 || code==orchard::explore) {
        Word virgin=!(a.appetite.sampled&(1ull<<x)),address=virgin+2*Word(body.receipt==2)+4*((motor>>2)&1);
        bool bite=q->policy!=2 && ((binding[i].genes.competition>>(8+address))&1);
        action=bite ? orchard::eat : orchard::physical_probe(a,x,key^motor,q->choice!=0);directed=false;
    }
    decisions[i]=site|(action<<5)|(Word(directed)<<8);
}
KERNEL void garden_settle(const CommonsSettings *q,SeedBed *beds,Forager *bodies,Reservoir *pools,Agency *animals,CropPolicy *policy,const Word *decisions) {
    Word i=thread_index();if (i>=q->count) return;Word island=i/32,lane=i&31;
    auto &a=bodies[i];auto &mind=animals[i];auto &w=beds[island].field;auto &pool=pools[island];
    Word day=a.day++,key=orchard::key(q->seed,island,day),d=decisions[i],site=d&31,op=(d>>5)&7;
    bool alive=a.energy!=0;Word target=alive ? site : 32+lane,group;
    asm("match.any.sync.b32 %0,%1,0xffffffff;":"=r"(group):"r"(target));
    Word start=key&31,rotated=(group>>start)|(group<<((32-start)&31));Word winner=(Word(__builtin_ctz(rotated))+start)&31;
    bool owns=alive && lane==winner;Word before=w.plots[site],after=before,receipt=0,spent=1;
    bool bite=owns && op==orchard::eat,ripe=bite && causal::evaluate(w.food,before);
    Word gained=0,eaten=0,regrown=0;
    for (Word bearing=0;bearing<4;++bearing) {
        Word supply=pool.stock[bearing]+q->rations;if (supply>3*q->rations) supply=3*q->rations;
        bool claim=ripe && (site>>3)==bearing;Word mask=meaning::ballot(claim),order=(mask>>start)|(mask<<((32-start)&31));
        Word ahead=Word(__builtin_popcount(order&((1u<<((lane+32-start)&31))-1)));
        Word n=Word(__builtin_popcount(mask)),taken=n<supply ? n : supply;eaten+=taken;
        if (claim && ahead<supply) gained=q->calories;
        if (!lane) {regrown+=supply-pool.stock[bearing];pool.stock[bearing]=supply-taken;}
    }
    Word event=0;
    if (alive) {
        ++a.lived;
        if (!owns || (ripe && !gained)) {++a.contested;receipt=3;}
        else {
            if (bite) {
                if (gained) {++a.meals;after=w.dormant;receipt=1;}else {++a.missed;receipt=2;}
            } else {++a.experiments;a.diagnostic+=(d>>8)&1;a.after_miss+=a.receipt==2;spent+=q->price;after=causal::physical(w,op,before);receipt=4;}
            event=1u<<31|op|(before<<3)|((bite ? Word(gained!=0) : after)<<9);
        }
    }
    meaning::warp_fence();if (owns) w.plots[site]=static_cast<unsigned char>(after);meaning::warp_fence();
    // A neighbouring worker is visible only through its actual movement and
    // its consequence. Contested food supplies no negative taste lesson.
    Word peer=meaning::broadcast(event,(lane+1)&31);
    if (alive && q->credit) for (Word k=0;k<1+q->observers;++k) {
        Word heard=k ? peer : event;if (!(heard>>31)) continue;
        bool changed=orchard::consequence(mind,heard&7,(heard>>3)&63,(heard>>9)&63,q->garden_model);
        if (changed) policy[i].dirty=1;++a.meal_updates;
    }
    if (alive) {
        a.receipt=receipt;a.energy+=gained;Word charged=a.energy<spent ? a.energy : spent;a.energy-=charged;a.expense+=charged;
        if (a.energy>=q->reserve+12) {Word clutch=12*((a.energy-q->reserve)/12);a.offspring+=clutch;a.energy-=clutch;}
    }
    Word from_dormancy=meaning::ballot(gained && before==w.dormant);
    if (!lane) {pool.consumed+=eaten;pool.regrown+=regrown;beds[island].dormant_meals+=Word(__builtin_popcount(from_dormancy));}
    // Light and moisture spread locally on the post-action chart. All lanes
    // read that chart before the synchronous environmental change.
    Word now=w.plots[lane],neighbour=meaning::broadcast(now,(lane+31)&31),draw=mix(key+lane*71693u);
    if (q->volatility && draw%q->volatility==0) {
        Word bit=1u<<((draw>>12)%traits);now=(now&~bit)|(neighbour&bit);
    }
    Word weather=meaning::ballot(now!=Word(w.plots[lane]));if (!lane) pool.switches+=Word(__builtin_popcount(weather));
    w.plots[lane]=static_cast<unsigned char>(now);if (alive) {mind.scenes[lane]=static_cast<unsigned char>(now);mind.observed|=1u<<lane;}
}
KERNEL void garden_census(const CommonsSettings *q,const Agency *animals,const CropPolicy *policies,Count *totals) {
    Word lane=thread_index();Count v[4]{};
    for (Word i=lane;i<q->count;i+=128) {
        auto &a=animals[i];v[0]+=a.appetite.bites;v[1]+=a.appetite.field.compiled!=0;v[2]+=policies[i].builds;
        for (auto &row:a.model.fields) for (auto &f:row) v[3]+=f.compiled!=0;
    }
    __attribute__((shared)) static Count sums[128];
    for (Word j=0;j<4;++j) {sums[lane]=v[j];asm volatile("bar.sync 0;":::"memory");
        for (Word k=64;k;k>>=1) {if (lane<k) sums[lane]+=sums[lane+k];asm volatile("bar.sync 0;":::"memory");}
        if (!lane) totals[j]=sums[0];asm volatile("bar.sync 0;":::"memory");}
}
