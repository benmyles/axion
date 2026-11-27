#include "axion/passage.hpp"

// Private foraging catchments. Selection sees actual material divided into
// offspring and continued life. The animal receives only its sensory history.
namespace {
template<int Tiles,int Level,bool Breed> AX_FN void migration_life(
        const Genome *genomes,const SynapticLayer *layers,const AssociationLayer *links,const OutcomeLayer *organs,
        const Word *living,Word survivors,Settings c,Travel cfg,Word rooms,Word vision,TravelArm arm,GrowthScore *output,
        Aptitude *fitness,Word first,Word count,const Maze *oracle,SpringChoice *probes,ForageLedger *activity) {
    Word id=index(),thread=id/Tiles,tile=id&(Tiles-1);
    if (thread>=count) return;
    Word episode=first+thread,group=episode/4,slot=0;
    if constexpr (Breed) { slot=group/cfg.sets;group%=cfg.sets; }
    Word key=mix(cfg.seed^group*0xa08bd2bdu),rotation=episode&3;
    if constexpr (!Breed) slot=living[mix(key^0xb7947cd3u)%survivors];
    Word sites=c.side*c.side,base=(id&31)&~(Tiles-1),mask=Word((1ull<<Tiles)-1)<<base;
    const Genome &g=genomes[slot];
    Traveller<Tiles,Level> a{g,develop(mix(g.development+tile*0x9e3779b9u)),{},{},tile,base,mask,c};
    if (arm==TravelArm::tonic && c.navigation) a.c.navigation=1;
    a.banks.load(g,nullptr,tile,true,layers ? layers+slot : nullptr,Level ? links+slot : nullptr,
                 Level==3 ? context_tail(links,sites)+slot : nullptr,
                 Level==3 && c.programs==2 ? expression_tail(links,sites)+slot : nullptr,c.polarity);
    if (c.navigation) { a.map.load(map_tail(organs,sites)[slot],tile,true);a.map.load_trace(organs,sites,slot,tile,true,c.navigation); }
    if (arm==TravelArm::silent) a.map.cut=MapCut::silent;
    if (arm==TravelArm::fixed) a.map.cut=MapCut::fixed;
    if (arm==TravelArm::one_pulse) a.map.cut=MapCut::one_pulse;
    if (arm==TravelArm::no_appetite) a.map.cut=MapCut::no_appetite;
    if (arm==TravelArm::raw) a.map.cut=MapCut::raw;
    if (arm==TravelArm::trace_fixed) a.map.cut=MapCut::trace_fixed;
    if (arm==TravelArm::immediate) a.map.cut=MapCut::immediate;
    if (arm==TravelArm::no_hold) a.map.cut=MapCut::no_hold;
    if (arm==TravelArm::no_novelty) a.map.cut=MapCut::no_novelty;
    CrossingWorld labels{key,0,0};
    Word span=rooms*cfg.rounds,pos=mix(key)&(rooms-1),energy=48+(key&31),stores=0,carbon=0;
    Word gait=cfg.foot ? 4 : 8;
    Word period=c.season ? c.season : 128,elapsed=0,epoch=0,handling=0,event=0,previous=0;
    Word chemistry=permutation(mix(key^0x75c9a5f9u),0),stock=0x20202020u;
    Word ticks=0,births=0,offspring=0,intake=0,converted=0,visited=0,familiar=0;
    Word wet=0,dry=0,leave_wet=0,leave_dry=0,returning=0,leave_returning=0,meals=0,arrival=0;
    for (Word t=0;t<span && energy;++t) {
        Word visible=pos;
        if (vision==1 && (pos&3)==3) visible=pos-2;
        if (vision==2 && (pos&3)) visible=pos&3;
        Word draw=mix(key^t*0x5865ba91u),tag=labels.cue(visible);
        if (previous && c.navigation && c.plastic)
            a.map.encounter(previous,1u<<tag,absorbed(event),tile);
        if constexpr (!Breed) if (oracle && arm==TravelArm::intact && t>=32 && ((t+rotation)&31)==0) {
            // Test alternatives from the very same body, brain and past. The
            // physical distance tables are consulted only after each choice.
            const auto &terrain=oracle[group];
            auto original=a.map;
            for (Word spring=0;spring<4;++spring) {
                Word site=(key+spring*((key>>11)|1))&(rooms-1),nutrient=((chemistry>>(2*spring))+epoch)&3;
                if (pos==site) continue;
                Word answers=best_grips(terrain,epoch&1,pos,site,rotation);
                if (answers&(answers-1)) continue;
                for (Word test=0;test<(c.navigation>=4 ? 10u : 6u);++test) {
                    a.map=original;
                    if (test==1 && c.navigation) { a.map.load(map_tail(organs,sites)[slot],tile,true);a.map.load_trace(organs,sites,slot,tile,true,c.navigation); }
                    if (test==2) a.map.cut=MapCut::silent;
                    if (test==3) a.map.cut=MapCut::one_pulse;
                    if (test==4) a.map.cut=MapCut::no_appetite;
                    if (test==6) a.map.cut=MapCut::raw;
                    if (test==7) a.map.cut=MapCut::immediate;
                    if (test==8) a.map.cut=MapCut::no_hold;
                    if (test==9) a.map.cut=MapCut::no_novelty;
                    Word demand=1u<<((nutrient+Word(test==5))&3);
                    Tissue copy=a.soma;
                    Word action=a.choose(copy,tag,0,draw,nullptr,demand,event);
                    if (!tile) {
                        bool correct=(action&gait) && (answers&(1u<<(action&3)));
                        bool far=terrain.distances[epoch&1][site][pos]>=3,known=familiar&(1u<<spring);
                        auto &r=probes[test];
#define HOME_ADD(f,n) asm volatile("red.global.add.u64 [%0],%1;"::"l"(&r.f),"l"(Count(n)):"memory")
                        HOME_ADD(items,1);HOME_ADD(correct,correct);HOME_ADD(far_items,far);HOME_ADD(far_correct,far && correct);
                        HOME_ADD(familiar_items,known);HOME_ADD(familiar_correct,known && correct);
#undef HOME_ADD
                    }
                }
            }
            a.map=original;
        }
        Word actual_need=appetite(stores,g.body),need=arm==TravelArm::wrong_goal ? ((actual_need<<1)|(actual_need>>3))&15 : actual_need;
        Word stamp=0,pins=a.choose(a.soma,tag,0,draw,&stamp,need,event);
        a.map.sense_event(1u<<tag,tile,base,mask);a.banks.remember(stamp);++ticks;visited|=1u<<pos;
        Word effort=population(pins&0x5c),cost=1+(effort ? effort-1 : 0)+Word(!(t&7) && living_tiles(Tiles,g.development)*32*microticks>512);
        energy-=lesser(cost,energy);if (!energy) break;
        Word food=0,busy=handling,well=0,previous_meals=familiar;
        if (handling) --handling;
        // Four independent, gradually refilling springs. No remote site or
        // food identity is present in the inputs at the time muscles fire.
        UNROLL
        for (Word k=0;k<4;++k) {
            Word place=(key+k*((key>>11)|1))&(rooms-1),material=((chemistry>>(2*k))+epoch)&3;
            if constexpr (!Breed) if (pos==place) well=1u<<k;
            Word reservoir=(stock>>(8*k))&255;
            reservoir=lesser(48,reservoir+1+((draw>>(2*k))&1));
            if (pos==place && (pins&8) && !busy) {
                Word room=pouch_space(stores,material,g.body);
                Word carbon_space=255-carbon;
                Word meal=lesser(lesser(reservoir,mouthful(g.body,c.husk)),room>carbon_space ? room : carbon_space);
                if (meal) {
                    stores+=lesser(meal,room)<<(8*material);carbon+=lesser(meal,carbon_space);
                    reservoir-=meal;intake+=meal;food=material+1;handling=handling_time(meal,c.husk);
                    familiar|=1u<<k;
                }
            }
            stock=(stock&~(255u<<(8*k)))|(reservoir<<(8*k));
        }
        Word fuel=release_energy(stores,g.body,1023-energy,carbon,true);energy+=fuel;converted+=fuel;
        Word copy_cost=fidelity_cost(g.reproduction,4,c.repair);
        if (energy>=64+(g.reproduction&63)+copy_cost) {
            energy-=copy_cost;
            Word fraction=3+((g.reproduction>>16)&3),share=energy*fraction/10,substrate=carbon*fraction/10;
            stores-=divide_stocks(stores,g.reproduction);carbon-=substrate;energy-=share;
            offspring+=share+substrate;++births;
        }
        if (c.navigation) previous=a.map.note(1u<<tag,pins);
        Word doors=door_word(pos,rooms,key,(epoch&1) && (pos&3)==((key>>19)&3));
        Word next=pins&gait ? exit_to(doors,pins,rotation) : pos;
        if constexpr (!Breed) if (activity && !tile) {
            bool moved=next!=pos,known=bool(well&previous_meals);
            wet+=well!=0;dry+=well==0;leave_wet+=well && moved;leave_dry+=!well && moved;
            returning+=known;leave_returning+=known && moved;meals+=food!=0;
            if (well && !arrival) arrival=t+1;
        }
        pos=next;
        event=receipt(0,0,food);
        if (++elapsed==period) { elapsed=0;epoch=(epoch+1)&3;familiar=0; }
    }
    if (tile) return;
    if constexpr (Breed) {
        auto *s=fitness[slot].score+cfg.offset;
        Word mass=64*offspring/span,viability=(32*(offspring+energy+carbon)+32*ticks)/span;
        asm volatile("red.global.add.u32 [%0],%1;"::"l"(s),"r"(mass):"memory");
        asm volatile("red.global.add.u32 [%0],%1;"::"l"(s+1),"r"(viability):"memory");
    } else {
        auto &r=output[Word(arm)];
#define MIG_ADD(f,n) asm volatile("red.global.add.u64 [%0],%1;"::"l"(&r.f),"l"(Count(n)):"memory")
        MIG_ADD(alive,energy!=0);MIG_ADD(ticks,ticks);MIG_ADD(births,births);MIG_ADD(offspring,offspring);MIG_ADD(absorbed,intake);
        MIG_ADD(converted,converted);MIG_ADD(terminal,energy);MIG_ADD(visited,population(visited));
#undef MIG_ADD
        if (activity) {
            auto &h=activity[Word(arm)];
#define BEHAVIOR_ADD(f,v) asm volatile("red.global.add.u64 [%0],%1;" :: "l"(&h.f),"l"(Count(v)):"memory")
            BEHAVIOR_ADD(at_spring,wet);BEHAVIOR_ADD(away,dry);BEHAVIOR_ADD(depart_spring,leave_wet);BEHAVIOR_ADD(depart_away,leave_dry);
            BEHAVIOR_ADD(returning,returning);BEHAVIOR_ADD(depart_returning,leave_returning);BEHAVIOR_ADD(meals,meals);
            BEHAVIOR_ADD(found,arrival!=0);BEHAVIOR_ADD(first_arrival,arrival);
#undef BEHAVIOR_ADD
        }
    }
}
}
#define MIG_ARGS const Genome *g,const SynapticLayer *d,const AssociationLayer *a,const OutcomeLayer *m,const Word *live,Word n,Settings c,Travel tune,Word rooms,Word vision,TravelArm arm,GrowthScore *r,Aptitude *score,Word first,Word count,const Maze *oracle,SpringChoice *probes,ForageLedger *activity
#define MIG_ENTRY(t,l) \
ENTRY void springs##l##_##t(MIG_ARGS) { migration_life<t,l,false>(g,d,a,m,live,n,c,tune,rooms,vision,arm,r,score,first,count,oracle,probes,activity); } \
ENTRY void migrants##l##_##t(MIG_ARGS) { migration_life<t,l,true>(g,d,a,m,live,n,c,tune,rooms,vision,arm,r,score,first,count,oracle,probes,activity); }
#define MIG_LEVEL(t) MIG_ENTRY(t,0) MIG_ENTRY(t,1) MIG_ENTRY(t,2) MIG_ENTRY(t,3)
MIG_LEVEL(2)
MIG_LEVEL(4)
MIG_LEVEL(8)
MIG_LEVEL(16)
MIG_LEVEL(32)
#undef MIG_ARGS
#undef MIG_ENTRY
#undef MIG_LEVEL
