// Separate the instrumental history from passive changes of food chemistry.
// Only the physical observations made in each stage reach the nervous system.
namespace {
struct FoodGarden {
    Word key,items,rotation,recipe;
    AX_FN Word object(Word item) const { return panel_tag(key,item); }
    AX_FN Word flavour(Word item,Word pins) const {
        Word surface=permutation(mix(key^(item*0xb5297a4du)^0x162d992du),0);
        return (pins&8) ? 1+((surface>>(2*(pins&3)))&3) : 0;
    }
    AX_FN Word food(Word f,bool changed) const {
        if (!f) return 0;
        Word address=(f-1+(changed ? 1+(key>>8)%3 : 0))&3;
        return 1+(((recipe>>(2*address))+rotation)&3);
    }
};
template<int Tiles,int Level> struct FoodPupil {
    Tissue state;
    Dendrites<Tiles,Level> banks;
    Prospects<Tiles> future;
    Word event=0,clock=0;
    AX_FN void pair(const Genome &g,Word flavour,Word food,Word tile,Word base,Word mask,Settings c) {
        event=receipt(0,flavour,food);
        future.observe(g,0,8,flavour,food,tile);
        neural<Tiles,false>(g,state,food_field(0x1070000,0,event),tile,base,mask,c.plastic);
        ++clock;
    }
    AX_FN Word probe(const Genome &g,const FoodGarden &world,Settings c,ValueArm arm,bool changed,
                     Word tile,Word base,Word mask,Word *sensitive=nullptr) const {
        auto view=future;
        if (arm==ValueArm::silent) view.cut=OutcomeCut::silent;
        if (arm==ValueArm::no_chain) view.cut=OutcomeCut::no_association;
        if (arm==ValueArm::erased) {
            UNROLL
            for (int i=0;i<view.count;++i) view.rows[i]=0u-((view.gene.wiring>>30)&1);
            view.connections=view.gene.gates&65535;
        }
        Word correct=0,switched=0;
        for (Word item=0;item<world.items;++item) {
            Word actions=0,tie=mix(world.key^item*0xc116ea6bu);
            Word shown=arm==ValueArm::wrong_object ? (item+1)&(world.items-1) : item;
            for (Word goal=0;goal<4;++goal) {
                Tissue copy=state;
                Word demand=1u<<((goal+Word(arm==ValueArm::wrong_need))&3);
                Word motor=take_action<Tiles>(g,copy,banks,world.object(shown),0,tie,tile,base,mask,c,false,nullptr,
                                              food_field(0x1070000,demand,event),nullptr,&view,demand);
                Word pins=effector(g,motor);
                correct+=world.food(world.flavour(item,pins),changed)==goal+1;
                if (pins&8) actions|=1u<<(pins&3);
            }
            switched+=Word(actions && (actions&(actions-1)));
        }
        if (sensitive) *sensitive=switched;
        return correct;
    }
};
template<int Tiles,int Level,bool Breed>
AX_FN void value_trial(const Genome *genomes,const SynapticLayer *layers,const AssociationLayer *links,const OutcomeLayer *organs,
                       const Word *living,Word survivors,Settings c,Valuation cfg,ValueArm arm,ValueScore *totals,
                       Aptitude *fitness,Word first,Word count) {
    Word id=index(), lane=id&(Tiles-1), in_batch=id/Tiles;
    if (in_batch>=count) return;
    Word trial=first+in_batch, group=trial/4, slot=0;
    Word candidate=group/cfg.sets;
    if constexpr (Breed) { slot=living ? living[candidate] : candidate; group%=cfg.sets; }
    Word key=mix(cfg.seed^group*0xfdd3d9c7u);
    if constexpr (!Breed) slot=living[mix(key^0x34bda571u)%survivors];
    Word base=(id&31)&~(Tiles-1),mask=Word((1ull<<Tiles)-1)<<base;
    const auto &g=genomes[slot];
    FoodGarden world{key,cfg.items,trial&3,permutation(mix(key^0xaecda715u),0)};
    FoodPupil<Tiles,Level> pupil;
    pupil.state=develop(mix(g.development+lane*0x9e3779b9u));
    pupil.banks.load(g,nullptr,lane,true,layers ? layers+slot : nullptr,Level ? links+slot : nullptr,
                     Level==3 ? context_tail(links,c.side*c.side)+slot : nullptr,
                     Level==3 && c.programs==2 ? expression_tail(links,c.side*c.side)+slot : nullptr,c.polarity);
    pupil.future.load(organs[slot],lane,true);
    if (arm==ValueArm::fixed || !c.plastic) pupil.future.cut=OutcomeCut::fixed;
    ValueCounts<Word> res{};
    if constexpr (!Breed) {
        res.naive=pupil.probe(g,world,c,arm,false,lane,base,mask);
        Word ports=0;
        UNROLL
        for (Word m=0;m<8;++m) if (effector(g,m)&8) ports|=1u<<(effector(g,m)&3);
        res.reachable=population(ports);
    }
    Word seen_low=0,seen_high=0;
    for (Word round=0;round<cfg.rounds;++round) {
        Word shuffle=mix(key^(round*0x946bd3c7u));
        for (Word t=0;t<cfg.items;++t) {
            Word item=((shuffle>>16)+(shuffle|1)*t)&(cfg.items-1), cue=world.object(item), stamp=0;
            Word m=take_action<Tiles>(g,pupil.state,pupil.banks,cue,0,mix(key+pupil.clock*0x629d2ff1u),lane,base,mask,c,false,nullptr,
                                      food_field(0x1070000,15,pupil.event),&stamp,&pupil.future,15);
            Word pins=effector(g,m),flavour=world.flavour(item,pins);
            Word food=cfg.inferred ? 0 : world.food(flavour,false);
            if (flavour) {
                Word bit=1u<<(4*(item&7)+(pins&3));
                if (item<8) seen_low|=bit; else seen_high|=bit;
                ++res.tasted;
            }
            if (arm==ValueArm::no_taste) { flavour=0; food=0; }
            pupil.banks.remember(stamp);
            if (food && c.plastic) pupil.banks.update(g,g.expression>>16,delayed(cue,m),true,lane);
            pupil.future.observe(g,pattern(cue),m,flavour,food,lane);
            pupil.event=receipt(0,flavour,food); ++pupil.clock;
        }
    }
    if constexpr (!Breed) {
        res.explored=pupil.probe(g,world,c,arm,false,lane,base,mask);
        res.actions=population(seen_low)+population(seen_high);
        res.trials=pupil.clock;
        for (Word k=0;k<cfg.items;++k) res.complete+=(((k<8 ? seen_low : seen_high)>>(4*(k&7)))&15)==15;
    }
    for (Word epoch=0;epoch<2;++epoch) {
        for (Word round=0;round<cfg.pairings;++round) {
            Word order=permutation(mix(key^round*0xa21d37fbu^epoch),0);
            for (Word j=0;j<4;++j) {
                Word f=1+((order>>(2*j))&3), food=world.food(f,epoch);
                if (arm==ValueArm::unpaired) food=world.food(1+((f-1+((key+round)&3))&3),epoch);
                if (arm==ValueArm::no_taste) { f=0; food=0; }
                pupil.pair(g,f,food,lane,base,mask,c);
            }
        }
        if (!epoch) {
            res.learned=pupil.probe(g,world,c,arm,false,lane,base,mask);
            if constexpr (!Breed) res.surprise=pupil.probe(g,world,c,arm,true,lane,base,mask);
        } else {
            res.changed=pupil.probe(g,world,c,arm,true,lane,base,mask,&res.sensitive);
            if constexpr (!Breed) res.old=pupil.probe(g,world,c,arm,false,lane,base,mask);
        }
    }
    if (lane) return;
    if constexpr (Breed) {
        auto *score=fitness[candidate].score+cfg.fitness_offset;
        Word scaled=128*res.learned/(4*cfg.items),adapted=128*res.changed/(4*cfg.items);
        asm volatile("red.global.add.u32 [%0], %1;" :: "l"(score),"r"(scaled):"memory");
        asm volatile("red.global.add.u32 [%0], %1;" :: "l"(score+1),"r"(adapted):"memory");
    } else {
        auto &out=totals[Word(arm)];
#define VALUE_ADD(f) asm volatile("red.global.add.u64 [%0], %1;" :: "l"(&out.f),"l"(Count(res.f)):"memory")
        VALUE_ADD(naive); VALUE_ADD(explored); VALUE_ADD(learned); VALUE_ADD(surprise); VALUE_ADD(changed); VALUE_ADD(old);
        VALUE_ADD(sensitive); VALUE_ADD(actions); VALUE_ADD(trials); VALUE_ADD(tasted); VALUE_ADD(reachable); VALUE_ADD(complete);
#undef VALUE_ADD
    }
}
}
#define VALUE_ARGS const Genome *g,const SynapticLayer *d,const AssociationLayer *a,const OutcomeLayer *p,const Word *live,Word n,Settings c,Valuation v,ValueArm arm,ValueScore *out,Aptitude *fitness,Word first,Word batch
#define VALUE_ENTRY(t,l) \
    ENTRY void evaluate##l##_##t(VALUE_ARGS) { value_trial<t,l,false>(g,d,a,p,live,n,c,v,arm,out,fitness,first,batch); } \
    ENTRY void appetition##l##_##t(VALUE_ARGS) { value_trial<t,l,true>(g,d,a,p,live,n,c,v,arm,out,fitness,first,batch); }
#define VALUE_LEVELS(t) VALUE_ENTRY(t,0) VALUE_ENTRY(t,1) VALUE_ENTRY(t,2) VALUE_ENTRY(t,3)
VALUE_LEVELS(2)
VALUE_LEVELS(4)
VALUE_LEVELS(8)
VALUE_LEVELS(16)
VALUE_LEVELS(32)
#undef VALUE_LEVELS
#undef VALUE_ENTRY
#undef VALUE_ARGS

namespace {
// Hash the inherited words alone. A repeated clone can occupy many sites.
template<class T> AX_FN void fingerprint_genes(Word &a,Word &b,T values) {
    struct Words { Word value[sizeof(T)/sizeof(Word)]; };
    static_assert(sizeof(Words)==sizeof(T));
    auto genes=__builtin_bit_cast(Words,values);
    UNROLL
    for (Word value:genes.value) { a=mix(a^value); b=mix(b+value+0x6bf17791u); }
}
}
ENTRY void pupil_sites(const Site *world,const Genome *genes,const SynapticLayer *d,const AssociationLayer *a,const OutcomeLayer *p,
                       const Word *living,Word survivors,Word samples,Word seed,Settings c,Word *slots,PupilSite *records) {
    Word id=index(); if (id>=samples) return;
    Word pos=living[(Word(Count(id)*survivors/samples)+mix(seed)%survivors)%survivors];
    Body body=world[pos].body; Word slot=body.slot,x=0x89e088abu,y=0x5b4f7a49u,sites=c.side*c.side;
    fingerprint_genes(x,y,genes[slot]);
    if (d) fingerprint_genes(x,y,d[slot].genes);
    if (a) { fingerprint_genes(x,y,a[slot].genes); fingerprint_genes(x,y,a[slot].convergence); }
    if (c.relations==3) fingerprint_genes(x,y,context_tail(a,sites)[slot].genes);
    if (c.programs==2) fingerprint_genes(x,y,expression_tail(a,sites)[slot].genes);
    fingerprint_genes(x,y,p[slot].genes);
    if (c.navigation) fingerprint_genes(x,y,map_tail(p,sites)[slot].genes);
    if (c.navigation>=4) fingerprint_genes(x,y,trace_tail(p,sites)[slot].genes);
    slots[id]=slot; records[id]={pos,slot,body.founder,body.generation,Count(x)<<32|y};
}
