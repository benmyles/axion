namespace {
template<int Tiles,int Level,bool Breed>
AX_FN void crossing_life(const Genome *genomes,const SynapticLayer *layers,const AssociationLayer *links,const OutcomeLayer *organs,
                         const Word *living,Word survivors,Settings c,Travel cfg,TravelArm arm,TravelScore *totals,
                         Aptitude *fitness,Word first,Word count) {
    Word id=index(),lane=id&(Tiles-1),member=id/Tiles;
    if (member>=count) return;
    Word episode=first+member,group=episode/4,slot=0;
    if constexpr (Breed) { slot=group/cfg.sets;group%=cfg.sets; }
    Word seed=mix(cfg.seed^group*0x772c91abu);
    if constexpr (!Breed) slot=living[mix(seed^0xb8975a1du)%survivors];
    Word base=(id&31)&~(Tiles-1),mask=Word((1ull<<Tiles)-1)<<base;
    const Genome &g=genomes[slot];Word sites=c.side*c.side;
    Traveller<Tiles,Level> animal{g,develop(mix(g.development+lane*0x9e3779b9u)),{},{},lane,base,mask,c};
    animal.motivated=cfg.motivation==2 ? (group&1) : cfg.motivation;animal.change_food=animal.motivated ? cfg.change_food : 0;
    if (arm==TravelArm::tonic && c.navigation) animal.c.navigation=1;
    animal.banks.load(g,nullptr,lane,true,layers ? layers+slot : nullptr,Level ? links+slot : nullptr,
                      Level==3 ? context_tail(links,sites)+slot : nullptr,
                      Level==3 && c.programs==2 ? expression_tail(links,sites)+slot : nullptr,c.polarity);
    if (c.navigation) { animal.map.load(map_tail(organs,sites)[slot],lane,true); animal.map.load_trace(organs,sites,slot,lane,true,c.navigation); }
    if (arm==TravelArm::silent) animal.map.cut=MapCut::silent;
    if (arm==TravelArm::fixed) animal.map.cut=MapCut::fixed;
    if (arm==TravelArm::one_pulse) animal.map.cut=MapCut::one_pulse;
    if (arm==TravelArm::no_appetite) animal.map.cut=MapCut::no_appetite;
    CrossingWorld world{seed,cfg.depth,episode&3,cfg.foot ? 4u : 8u};
    Word history=cfg.history==3 ? mix(seed^0x72c3c697u)%3 : cfg.history;
    if (history==1) animal.earlier_life(seed^0x2b05f7a9u);
    Word naive=0,surprise=0,old=0,moves=0,reached=0,redirected=0,tried=0,observed=0,visited=0,informed=0;
    if constexpr (!Breed) naive=animal.probe(world,false,arm);
    animal.explore(world,cfg.rounds,false,arm,observed,tried,visited);
    if (animal.motivated) animal.meals(world,false,arm);
    if (history==2) animal.earlier_life(seed^0x2b05f7a9u);
    if (arm==TravelArm::erased && c.navigation) { animal.map.load(map_tail(organs,sites)[slot],lane,true); animal.map.load_trace(organs,sites,slot,lane,true,c.navigation); }
    Word learned=animal.probe(world,false,arm);
    if constexpr (!Breed) { surprise=animal.probe(world,true,arm); reached=animal.walk(world,false,arm,moves); }
    if (animal.change_food!=1) animal.explore(world,cfg.rounds,true,arm,observed,tried,visited);
    if (animal.change_food) animal.meals(world,true,arm);
    if (arm==TravelArm::erased && c.navigation) { animal.map.load(map_tail(organs,sites)[slot],lane,true); animal.map.load_trace(organs,sites,slot,lane,true,c.navigation); }
    Word changed=animal.probe(world,true,arm,Breed ? nullptr : &informed,visited);
    if constexpr (!Breed) { old=animal.probe(world,false,arm);redirected=animal.walk(world,true,arm,moves); }
    if (lane) return;
    if constexpr (Breed) {
        auto *score=fitness[slot].score+cfg.offset;
        asm volatile("red.global.add.u32 [%0], %1;" :: "l"(score),"r"(learned*8):"memory");
        asm volatile("red.global.add.u32 [%0], %1;" :: "l"(score+1),"r"(changed*8):"memory");
    } else {
        auto &v=totals[Word(arm)];
#define TRAVEL_ADD(f,x) asm volatile("red.global.add.u64 [%0], %1;" :: "l"(&v.f),"l"(Count(x)):"memory")
        TRAVEL_ADD(naive,naive);TRAVEL_ADD(first,learned);TRAVEL_ADD(surprise,surprise);TRAVEL_ADD(changed,changed);
        TRAVEL_ADD(old,old);TRAVEL_ADD(steps,moves);TRAVEL_ADD(arrived,reached);TRAVEL_ADD(redirected,redirected);
        TRAVEL_ADD(tried,tried);TRAVEL_ADD(observed,observed);
        TRAVEL_ADD(seen_answers,informed);TRAVEL_ADD(seen_items,4*population(visited));
#undef TRAVEL_ADD
    }
}
}
#define TRAVEL_ARGS const Genome *g,const SynapticLayer *d,const AssociationLayer *a,const OutcomeLayer *m,const Word *live,Word n,Settings c,Travel tune,TravelArm arm,TravelScore *r,Aptitude *fitness,Word first,Word count
#define TRAVEL_RUN(t,l) \
ENTRY void crossing##l##_##t(TRAVEL_ARGS) { crossing_life<t,l,false>(g,d,a,m,live,n,c,tune,arm,r,fitness,first,count); } \
ENTRY void navigator##l##_##t(TRAVEL_ARGS) { crossing_life<t,l,true>(g,d,a,m,live,n,c,tune,arm,r,fitness,first,count); }
#define TRAVEL_LEVEL(t) TRAVEL_RUN(t,0) TRAVEL_RUN(t,1) TRAVEL_RUN(t,2) TRAVEL_RUN(t,3)
TRAVEL_LEVEL(2)
TRAVEL_LEVEL(4)
TRAVEL_LEVEL(8)
TRAVEL_LEVEL(16)
TRAVEL_LEVEL(32)
#undef TRAVEL_ARGS
#undef TRAVEL_RUN
#undef TRAVEL_LEVEL
