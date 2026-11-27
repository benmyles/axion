// A solitary common garden. Only material passed to offspring and continued
// life determine reproductive selection here. No devaluation probes enter it.
namespace {
template<int Tiles,int Level,bool Breed>
AX_FN void forage_life(const Genome *genes,const SynapticLayer *layers,const AssociationLayer *links,const OutcomeLayer *organs,
                       const Word *live,Word survivors,Settings c,Valuation cfg,ValueArm arm,GrowthScore *output,
                       Aptitude *fitness,Word first,Word count) {
    Word thread=index(),within=thread/Tiles,tile=thread&(Tiles-1);
    if (within>=count) return;
    Word id=first+within, group=id/4,slot=0;
    if constexpr (Breed) { slot=group/cfg.sets; group%=cfg.sets; }
    Word key=mix(cfg.seed^group*0x34b94c81u),base=(thread&31)&~(Tiles-1),mask=Word((1ull<<Tiles)-1)<<base;
    if constexpr (!Breed) slot=live[mix(key^0x9d11aef5u)%survivors];
    const Genome &g=genes[slot];
    Tissue state=develop(mix(g.development+tile*0x9e3779b9u));
    Dendrites<Tiles,Level> banks;
    banks.load(g,nullptr,tile,true,layers ? layers+slot : nullptr,Level ? links+slot : nullptr,
               Level==3 ? context_tail(links,c.side*c.side)+slot : nullptr,
               Level==3 && c.programs==2 ? expression_tail(links,c.side*c.side)+slot : nullptr,c.polarity);
    Prospects<Tiles> senses; senses.load(organs[slot],tile,true);
    if (arm==ValueArm::silent) senses.cut=OutcomeCut::silent;
    if (arm==ValueArm::fixed || !c.plastic) senses.cut=OutcomeCut::fixed;
    if (arm==ValueArm::no_chain) senses.cut=OutcomeCut::no_association;
    Plant stand{mix(key+1),mix(key+2),mix(key+3),mix(key+4)};
    Word recipes=0;
    UNROLL
    for (Word k=0;k<4;++k) recipes|=permutation(mix(key^k*0x7e56b283u^0x58ec2a1fu),0)<<(8*k);
    Word energy=48+(key&31), stores=0, carbon=0, event=0, ticks=0, births=0,offspring=0,intake=0,ambient_food=0,burned=0;
    Word length=cfg.items*cfg.rounds,period=c.season ? c.season : 24,elapsed=0,phase=0,limiting=id&3;
    PeripheralHistory history{};
    Word handling=0;
    Word lag=c.digestion ? conduction(g) : 1;
    for (Word t=0;t<length && energy;++t) {
        Word draw=mix(key^t*0x632bd34bu);
        if (!elapsed) {
            Word season=mix(key^((t/period)*0xe121a9abu));
            phase=season&(c.climates ? c.climates-1 : 0);
            limiting=((season>>24)+(id&3))&3;
        }
        if (++elapsed==period) elapsed=0;
        Word shape=draw&(cfg.items-1), texture=cfg.inferred ? (draw>>8)&(cfg.items-1) : 0;
        Word tag=cfg.inferred ? pair_cue(attribute(key,shape,0),attribute(mix(key),texture,1)) : panel_tag(key,shape);
        Word actual_need=appetite(stores,g.body);
        Word demand=arm==ValueArm::wrong_need ? ((actual_need<<1)|(actual_need>>3))&15 : actual_need;
        Word stamp=0,field=food_field((Word(energy<24)<<19)|0x70000u,demand,event);
        Word motor=take_action<Tiles>(g,state,banks,tag,0,draw,tile,base,mask,c,false,nullptr,field,&stamp,&senses,demand);
        Word pins=effector(g,motor),before=energy,effort=population(pins&0x5c);
        Word cost=1+Word((pins&4) && !(t&3))+Word(!(t&7) && living_tiles(Tiles,g.development)*32*microticks>512)+(effort ? effort-1 : 0);
        ++ticks; energy-=lesser(energy,cost);
        if (!energy) break;
        Word ground=12+((draw>>16)&31),f=0,food=0;
        Word busy=handling;
        if (handling) --handling;
        if (pins&8) {
            Word code=plant_grip(stand,shape+cfg.items*texture,cfg.items);
            f=surface_flavour(stand,code,pins);
            Word kind=material_code(recipes,phase,f), space=pouch_space(stores,kind,g.body);
            Word room=c.balanced==2 && 255-carbon>space ? 255-carbon : space;
            Word meal=busy ? 0 : lesser(lesser(ground,mouthful(g.body,c.husk)),room);
            if (meal) handling=handling_time(meal,c.husk);
            if (c.balanced==2) carbon+=lesser(meal,255-carbon);
            stores+=lesser(meal,space)<<(8*kind); intake+=meal; ground-=meal;
            if (meal) food=kind+1;
        }
        if (c.leach) stores-=lesser(c.leach,(stores>>(8*limiting))&255)<<(8*limiting);
        if (c.seep) {
            UNROLL
            for (Word j=0;j<4;++j) {
                Word k=(draw+j)&3;
                Word n=k==limiting ? 0 : lesser(lesser(c.seep,ground),pouch_space(stores,k,g.body));
                stores+=n<<(8*k); if (c.balanced!=2) ground-=n; ambient_food+=n;
            }
        }
        Word fuel=release_energy(stores,g.body,1023-energy,carbon,c.balanced==2);
        energy+=fuel; burned+=fuel;
        senses.observe(g,pattern(tag),motor,f,food,tile);
        banks.remember(stamp); history.append(delayed(tag,motor));
        if (c.plastic && ticks>=lag) banks.update(g,g.expression>>16,history.at(lag),energy>before,tile,true,lag);
        event=receipt(0,f,food);
        Word cost_of_copy=fidelity_cost(g.reproduction,4,c.repair);
        if (energy>=64+(g.reproduction&63)+cost_of_copy) {
            energy-=cost_of_copy;
            Word share=energy*(3+((g.reproduction>>16)&3))/10, basket=divide_stocks(stores,g.reproduction);
            energy-=share; stores-=basket; offspring+=share; ++births;
            Word substrate=carbon*(3+((g.reproduction>>16)&3))/10; carbon-=substrate; offspring+=substrate;
            UNROLL
            for (Word j=0;j<4 && c.balanced!=2;++j) offspring+=(basket>>(8*j))&255;
        }
    }
    if (tile) return;
    if constexpr (Breed) {
        Word tissue=0;
        UNROLL
        for (Word j=0;j<4;++j) tissue+=(stores>>(8*j))&255;
        if (c.balanced==2) tissue=carbon;
        Word output1=(64*offspring)/length;
        Word output2=(32*(offspring+energy+tissue)+32*ticks)/length;
        auto *score=fitness[slot].score+cfg.fitness_offset;
        asm volatile("red.global.add.u32 [%0], %1;" :: "l"(score),"r"(output1):"memory");
        asm volatile("red.global.add.u32 [%0], %1;" :: "l"(score+1),"r"(output2):"memory");
    } else {
        auto &out=output[Word(arm)];
#define GROW(f,v) asm volatile("red.global.add.u64 [%0], %1;" :: "l"(&out.f),"l"(Count(v)):"memory")
        GROW(alive,energy!=0); GROW(ticks,ticks); GROW(births,births); GROW(offspring,offspring); GROW(absorbed,intake);
        GROW(seep,ambient_food); GROW(converted,burned); GROW(terminal,energy);
#undef GROW
    }
}
}
#define GROW_ARGS const Genome *g,const SynapticLayer *d,const AssociationLayer *a,const OutcomeLayer *p,const Word *live,Word n,Settings c,Valuation v,ValueArm arm,GrowthScore *out,Aptitude *fitness,Word first,Word batch
#define GROW_ENTRY(t,l) \
ENTRY void broodfood##l##_##t(GROW_ARGS) { forage_life<t,l,true>(g,d,a,p,live,n,c,v,arm,out,fitness,first,batch); } \
ENTRY void growfood##l##_##t(GROW_ARGS) { forage_life<t,l,false>(g,d,a,p,live,n,c,v,arm,out,fitness,first,batch); }
#define GROW_LEVELS(t) GROW_ENTRY(t,0) GROW_ENTRY(t,1) GROW_ENTRY(t,2) GROW_ENTRY(t,3)
GROW_LEVELS(2)
GROW_LEVELS(4)
GROW_LEVELS(8)
GROW_LEVELS(16)
GROW_LEVELS(32)
#undef GROW_LEVELS
#undef GROW_ENTRY
#undef GROW_ARGS
