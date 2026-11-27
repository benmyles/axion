namespace {
// Each aligned 8 by 4 patch contains all four springs in a single climate.
// Ten steps therefore bound the search even across a climate boundary.
AX_FN Word toward_springs(Word pos,Word tick,Settings c) {
    Word best[4]{11,11,11,11},bearings[4]{};
    for (int dy=-10;dy<=10;++dy) for (int dx=-10;dx<=10;++dx) {
        Word distance=(dx<0 ? -dx : dx)+(dy<0 ? -dy : dy);
        if (distance>10) continue;
        Word place=cell_at((pos&(c.side-1))+dx,pos/c.side+dy,c.side);
        Word found=aquifer(place,local_climate(place,tick,c,nullptr).low,c);
        if (!found) continue;
        Word k=found-1,direction=Word(dy<0)|(Word(dx>0)<<1)|(Word(dy>0)<<2)|(Word(dx<0)<<3);
        if (distance<best[k]) { best[k]=distance;bearings[k]=direction; }
        else if (distance==best[k]) bearings[k]|=direction;
    }
    return bearings[0]|(bearings[1]<<4)|(bearings[2]<<8)|(bearings[3]<<12);
}
template<int Tiles,int Level>
AX_FN void appetite_probe(const Site *sites,const Genome *genomes,const Tissue *tissue,const Contacts *contacts,
                          const unsigned char *pods,const SynapticLayer *layers,const AssociationLayer *links,const OutcomeLayer *organs,
                          Plant *plants,const DelayLine *history,const Word *positions,Word survivors,Word samples,
                          Word tick,Word seed,Word repeats,AppetiteArm arm,Settings c,AppetiteScore *result) {
    Word thread=index(),id=thread/Tiles,tile=thread&(Tiles-1);
    if (id>=samples) return;
    Word pos=positions[(Word(Count(id)*survivors/samples)+mix(seed)%survivors)%survivors], key=mix(seed^pos);
    Word base=(thread&31)&~(Tiles-1),mask=Word((1ull<<Tiles)-1)<<base;
    Body b=sites[pos].body; const Genome &g=genomes[b.slot];
    bool cohort=treatment(b.founder,c);
    c.plastic=c.plastic && (c.contest!=Contest::plasticity || cohort);
    c.random_actions=c.random_actions || (c.contest==Contest::agency && !cohort);
    bool fresh=b.born==tick || arm==AppetiteArm::innate;
    Tissue state=fresh ? develop(mix(g.development+0x9e3779b9u*tile)) : tissue[b.slot*Tiles+tile];
    Dendrites<Tiles,Level> banks;
    banks.load(g,contacts+b.slot,tile,fresh,layers ? layers+b.slot : nullptr,Level ? links+b.slot : nullptr,
               Level==3 ? context_tail(links,c.side*c.side)+b.slot : nullptr,
               Level==3 && c.programs==2 ? expression_tail(links,c.side*c.side)+b.slot : nullptr,c.polarity);
    if constexpr (Level) if (!cohort && c.contest==Contest::recurrent_paths) banks.control=PathControl::direct_only;
    if constexpr (Level>=2) if (!cohort && c.contest==Contest::distal_paths) banks.control=PathControl::short_range;
    if constexpr (Level==3) if (!cohort) {
        if (c.contest==Contest::contextual_memory) banks.context_cut=ContextCut::pooled;
        if (c.contest==Contest::contextual_evidence) banks.context_cut=ContextCut::blind;
    }
    Prospects<Tiles> predictions;
    predictions.load(organs[b.slot],tile,fresh || arm==AppetiteArm::reset);
    if (arm==AppetiteArm::silent || (c.contest==Contest::prospective && !treatment(b.founder,c))) predictions.cut=OutcomeCut::silent;
    if (c.contest==Contest::outcome_association && !treatment(b.founder,c)) predictions.cut=OutcomeCut::no_association;
    if (!fresh && c.plastic) {
        Word prior=contacts[b.slot].previous,r=organs[b.slot].associations,lag=c.digestion ? conduction(g) : 1;
        if (prior && arm!=AppetiteArm::reset && (c.contest!=Contest::prospective_plasticity || treatment(b.founder,c)))
            predictions.observe(g,pattern(earlier_cue(prior)),(prior>>5)&7,tasted(r),absorbed(r),tile);
        if (tick-b.born>=lag && (c.contest!=Contest::dendritic_plasticity || cohort))
            banks.update(g,g.expression>>16,history ? history[b.slot].discharge[(tick-lag)&7] : prior,b.recent==1,tile,
                         c.contest!=Contest::metaplasticity || cohort,lag);
    }
    Word source=arm==AppetiteArm::remote ? (pos+(c.side/2)*(c.side+1))&(c.side*c.side-1) : pos;
    MapCircuit<Tiles> map;
    Word place=c.navigation ? landmark(pos,c) : 0;
    if (c.navigation) {
        const auto &local=map_tail(organs,c.side*c.side)[b.slot];map.load(local,tile,fresh || arm==AppetiteArm::map_reset);map.load_trace(organs,c.side*c.side,b.slot,tile,fresh || arm==AppetiteArm::map_reset || arm==AppetiteArm::trace_reset,c.navigation);
        if (!cohort) {
            if (c.contest==Contest::navigation_signal) map.cut=MapCut::silent;
            if (c.contest==Contest::navigation_plasticity) map.cut=MapCut::fixed;
            if (c.contest==Contest::navigation_range) map.cut=MapCut::one_pulse;
            if (c.contest==Contest::temporal_signal) map.cut=MapCut::raw;
            if (c.contest==Contest::temporal_plasticity) map.cut=MapCut::trace_fixed;
            if (c.contest==Contest::temporal_span) map.cut=MapCut::immediate;
        }
        if (arm==AppetiteArm::raw || arm==AppetiteArm::trace_reset) map.cut=MapCut::raw;
        if (arm==AppetiteArm::immediate) map.cut=MapCut::immediate;
        if (arm==AppetiteArm::no_hold) map.cut=MapCut::no_hold;
        if (arm==AppetiteArm::map_silent) map.cut=MapCut::silent;
        if (arm==AppetiteArm::one_pulse) map.cut=MapCut::one_pulse;
        if (arm==AppetiteArm::no_appetite) map.cut=MapCut::no_appetite;
        if (!fresh && c.plastic && arm!=AppetiteArm::map_reset) map.encounter(local.previous,place,absorbed(organs[b.slot].associations),tile);
    }
    const Plant &plant=plants[source];
    Word region=territory(source,c),phase=local_climate(source,tick,c,nullptr).low;
    Word sites_count=c.side*c.side, reaction=phase ? reinterpret_cast<Word *>(plants+sites_count)[(phase-1)*sites_count+source] : plant.reaction;
    Word chemistry=food_chemistry(plants,sites_count,c.climates)[source];
    Word field=sense(sites,pos,tick,c), correct=0, possible=0, responsive=0;
    Word bearings=0,vacant=0,away=0,moving=0,closer=0,open=0,passable=0;
    if (c.springs && !tile) {
        bearings=toward_springs(pos,tick,c);
        Word k=((pos-(tick&1))&1)|(((pos/c.side-((tick^(tick>>1))&1))&1)<<1);
        for (Word d=0;d<4;++d) if (inside(k,d)>=0 && !sites[neighbor(pos,d,c.side)].body.energy) vacant|=1u<<d;
    }
    for (Word j=0;j<repeats;++j) {
        Word draw=mix(key^j*0x58fdde99u);
        Word pod=j ? (c.fruit==Fruit::inherited ? plant_fruit(plant,draw,c.species) : draw&(c.species*c.species-1)) : pods[source];
        Word class_code=plant_response(plant,pod,c.species,reaction), tag=visible_pod(region,pod,c);
        Word feasible=0,selected=0;
        UNROLL
        for (Word m=0;m<8;++m) {
            Word pins=effector(g,m);
            if (pins&8) {
                Word kind=material_code(chemistry,phase,surface_flavour(plant,class_code,pins));
                if (!c.springs || aquifer(source,phase,c)==kind+1) feasible|=1u<<kind;
            }
        }
        for (Word goal=0;goal<4;++goal) {
            Tissue copy=state;
            Word need=1u<<((goal+Word(arm==AppetiteArm::wrong_need))&3);
            Word tie=c.springs && !j ? mix(tick*0x9e3779b9u+b.slot+c.seed) : draw;
            Word motor=take_action<Tiles>(g,copy,banks,tag,0,tie,tile,base,mask,c,false,nullptr,
                food_field(field,need,organs[b.slot].associations),nullptr,&predictions,need,c.navigation ? &map : nullptr,place);
            Word pins=effector(g,motor);
            if (c.springs && !tile && !j) {
                Word directions=(bearings>>(4*goal))&15,chosen=(pins&4) ? 1u<<(pins&3) : 0;
                if (directions) {
                    ++away;moving+=chosen!=0;closer+=(directions&chosen)!=0;
                    open+=(directions&vacant)!=0;passable+=(directions&vacant&chosen)!=0;
                }
            }
            if (pins&8) {
                selected|=1u<<(pins&3);
                correct+=material_code(chemistry,phase,surface_flavour(plant,class_code,pins))==goal && (!c.springs || aquifer(source,phase,c)==goal+1);
            }
        }
        possible+=population(feasible); responsive+=Word(selected && (selected&(selected-1)));
    }
    if (tile) return;
    auto &r=result[Word(arm)];
#define APPETITE_ADD(f,v) asm volatile("red.global.add.u64 [%0], %1;" :: "l"(&r.f),"l"(Count(v)):"memory")
    APPETITE_ADD(correct,correct); APPETITE_ADD(possible,possible); APPETITE_ADD(responsive,responsive);
    APPETITE_ADD(sampled,1); APPETITE_ADD(age,tick-b.born); APPETITE_ADD(stocked,sites[source].food!=0);
    if (c.springs) {
        APPETITE_ADD(away,away);APPETITE_ADD(moving,moving);APPETITE_ADD(closer,closer);
        APPETITE_ADD(open,open);APPETITE_ADD(passable,passable);
    }
#undef APPETITE_ADD
}
}
#define APP_ARGS const Site *w,const Genome *g,const Tissue *s,const Contacts *d,const unsigned char *p,const SynapticLayer *l,const AssociationLayer *a,const OutcomeLayer *o,Plant *f,const DelayLine *h,const Word *pos,Word survivors,Word count,Word t,Word seed,Word repeats,AppetiteArm arm,Settings c,AppetiteScore *result
#define APP(width,level) ENTRY void appetite##level##_##width(APP_ARGS) { appetite_probe<width,level>(w,g,s,d,p,l,a,o,f,h,pos,survivors,count,t,seed,repeats,arm,c,result); }
#define APP_LEVELS(t) APP(t,0) APP(t,1) APP(t,2) APP(t,3)
APP_LEVELS(2)
APP_LEVELS(4)
APP_LEVELS(8)
APP_LEVELS(16)
APP_LEVELS(32)
#undef APP_LEVELS
#undef APP
#undef APP_ARGS
