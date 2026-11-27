namespace {
template<int Tiles,int Level>
AX_FN void maze_life(const Genome *genomes,const SynapticLayer *layers,const AssociationLayer *links,const OutcomeLayer *organs,
                     const Word *living,Word survivors,Settings c,const Maze *worlds,Travel cfg,Word rooms,bool roam,
                     TravelArm arm,MazeScore *result,Word first,Word count) {
    Word id=index(),lane=id&(Tiles-1),subject=id/Tiles;
    if (subject>=count) return;
    Word episode=first+subject,group=episode/4,rotation=episode&3;
    Word key=maze_seed(cfg.seed,group),base=(id&31)&~(Tiles-1),mask=Word((1ull<<Tiles)-1)<<base;
    Word slot=living[mix(key^0x2b8c504bu)%survivors],sites=c.side*c.side;
    const auto &map=worlds[group];const auto &g=genomes[slot];
    CrossingWorld labels{key,0,rotation};
    Traveller<Tiles,Level> a{g,develop(mix(g.development+lane*0x9e3779b9u)),{},{},lane,base,mask,c};
    Word gait=cfg.foot ? 4 : 8;
    bool food=cfg.motivation==2 ? (group&1) : cfg.motivation;
    bool change_food=cfg.change_food && food,change_doors=cfg.change_food!=1 || !food;
    if (arm==TravelArm::tonic && c.navigation) a.c.navigation=1;
    a.banks.load(g,nullptr,lane,true,layers ? layers+slot : nullptr,Level ? links+slot : nullptr,
                 Level==3 ? context_tail(links,sites)+slot : nullptr,
                 Level==3 && c.programs==2 ? expression_tail(links,sites)+slot : nullptr,c.polarity);
    if (c.navigation) { a.map.load(map_tail(organs,sites)[slot],lane,true); a.map.load_trace(organs,sites,slot,lane,true,c.navigation); }
    if (arm==TravelArm::silent) a.map.cut=MapCut::silent;
    if (arm==TravelArm::fixed) a.map.cut=MapCut::fixed;
    if (arm==TravelArm::one_pulse) a.map.cut=MapCut::one_pulse;
    if (arm==TravelArm::no_appetite) a.map.cut=MapCut::no_appetite;
    Word history=cfg.history==3 ? mix(key^0x72c3c697u)%3 : cfg.history;
    if (history==1) a.earlier_life(key^0x2b05f7a9u);
    Word previous=0,distinct=0,visits=0; // one four-bit sensory-action mask per physical row
    Word witnessed[32/Tiles]{};
    MazeScore score{};
    for (Word epoch=0;epoch<2;++epoch) {
        Word pos=mix(key^0x51e923b3u)&(rooms-1);
        for (Word round=0;round<(epoch && !change_doors ? 0 : cfg.rounds);++round) {
            Word order=mix(key^round*0x4d6578abu);
            for (Word step=0;step<rooms;++step) {
                bool walking=roam && !epoch;
                if (!walking) pos=(order+step*((order>>8)|1))&(rooms-1);
                if (epoch && !(map.junctions&(1u<<pos))) continue;
                Word tag=labels.cue(pos),stamp=0,tie=mix(key^round*0x49d2f8bdu^step*0x4517e9adu);
                // In a continuous walk, the next observation arrives at the next decision.
                if (walking && c.navigation && c.plastic)
                    a.map.encounter(previous,1u<<labels.cue(arm==TravelArm::misbound ? (pos+1)&(rooms-1) : pos),0,lane);
                Word pins=a.choose(a.soma,tag,0,tie,&stamp);a.map.sense_event(1u<<tag,lane,base,mask);
                if (epoch && arm==TravelArm::escorted) pins=gait|((pos+round)&3);
                Word actual=passage_to(epoch && arm!=TravelArm::old_exits ? map.changed : map.original,pos,pins,rotation,gait);
                if (!epoch) {
                    visits|=1u<<pos;
                    UNROLL
                    for (Word j=0;j<32/Tiles;++j) if (pos==lane+j*Tiles && (pins&gait)) witnessed[j]|=1u<<(pins&3);
                }
                a.banks.remember(stamp);
                Word sensory=arm==TravelArm::misbound ? (actual+1)&(rooms-1) : actual;
                if (walking) {
                    if (c.navigation) previous=a.map.note(1u<<tag,pins);
                    pos=actual;
                } else {
                    if (c.navigation && c.plastic) a.map.encounter(a.map.note(1u<<tag,pins),1u<<labels.cue(sensory),0,lane);
                    a.map.sense_event(1u<<labels.cue(sensory),lane,base,mask);
                    neural<Tiles,false>(g,a.soma,1u<<labels.cue(sensory),lane,base,mask,c.plastic);
                }
            }
        }
        if (roam && !epoch && c.navigation && c.plastic) a.map.encounter(previous,1u<<labels.cue(arm==TravelArm::misbound ? (pos+1)&(rooms-1) : pos),0,lane);
        Word wells=0;
        if (food) for (Word v=0;v<4;++v) {
            Word site=(key+v*((key>>12)|1))&(rooms-1);wells|=1u<<site;
            if (!epoch || change_food) a.taste(labels.cue(site),labels.nutrient(v+Word(arm==TravelArm::unpaired_meals),
                                                      epoch && change_food && arm!=TravelArm::old_exits));
        }
        if (!epoch && history==2) a.earlier_life(key^0x2b05f7a9u);
        if (arm==TravelArm::erased && c.navigation) { a.map.load(map_tail(organs,sites)[slot],lane,true); a.map.load_trace(organs,sites,slot,lane,true,c.navigation); }
        for (Word q=0;q<(food ? 4u : 8u);++q) {
            Word goal=(key+q*((key>>12)|1))&(rooms-1);
            Word kind=labels.nutrient(q,false)-1;
            Word new_goal=change_food ? (key+(labels.food_site(kind,true)&3)*((key>>12)|1))&(rooms-1) : goal;
            Word desired=food ? 0 : 1u<<labels.cue(arm==TravelArm::wrong_goal ? (goal+1)&(rooms-1) : goal);
            Word need=food ? 1u<<((kind+Word(arm==TravelArm::wrong_goal))&3) : 0;
            Word new_geometry=Word(change_doors);
            for (Word root=0;root<rooms;++root) {
                if (root==goal || root==new_goal) continue;
                Word before=best_grips(map,0,root,goal,rotation),after=best_grips(map,new_geometry,root,new_goal,rotation);
                bool unique=!(before&(before-1)), changed=unique && !(after&(after-1)) && !(before&after) && !(((change_food ? wells : 0u)|(change_doors ? map.junctions : 0u))&(1u<<root));
                if (epoch ? !changed : !unique) continue;
                Tissue copy=a.soma;
                Word output=a.choose(copy,labels.cue(root),desired,mix(key^root*0x14e325du),nullptr,need);
                Word response=(output&gait) ? 1u<<(output&3) : 0;
                bool old=(response&before)!=0,revised=(response&after)!=0;
                if (!epoch) {
                    ++score.questions;score.first+=old;
                    Word dist=map.distances[0][goal][root];
                    if (dist==1) { ++score.near_items;score.near_first+=old; }
                    if (dist>=3) { ++score.far_items;score.far_first+=old; }
                    if (changed) { ++score.changed_items;score.ready+=old;score.surprise+=revised; }
                } else { score.turned+=revised;score.obsolete+=old; }
            }
            if (epoch) {
                Word source=(goal+rooms/2)&(rooms-1),place=source,length=0;
                Tissue copy=a.soma;
                while (place!=new_goal && length<rooms) {
                    Word pins=a.choose(copy,labels.cue(place),desired,mix(key^q*0xb56718dfu^length*0x5393f7u),nullptr,need);
                    place=passage_to(change_doors ? map.changed : map.original,place,pins,rotation,gait);++length;
                }
                Word best=map.distances[new_geometry][new_goal][source];
                ++score.journeys;score.reached+=place==new_goal;score.shortest+=length==best;score.steps+=length;score.length+=best;
            }
        }
    }
    UNROLL
    for (Word bank:witnessed) distinct+=population(bank);
    score.distinct=group_sum<Tiles>(distinct,lane,base,mask);score.coverage=population(visits);score.seen=rooms*cfg.rounds;
    if (lane) return;
    auto &out=result[Word(arm)];
#define MAZE_ADD(f) asm volatile("red.global.add.u64 [%0], %1;" :: "l"(&out.f),"l"(score.f):"memory")
    MAZE_ADD(questions);MAZE_ADD(first);MAZE_ADD(near_items);MAZE_ADD(near_first);MAZE_ADD(far_items);MAZE_ADD(far_first);
    MAZE_ADD(changed_items);MAZE_ADD(ready);MAZE_ADD(surprise);MAZE_ADD(turned);MAZE_ADD(obsolete);
    MAZE_ADD(journeys);MAZE_ADD(reached);MAZE_ADD(shortest);MAZE_ADD(steps);MAZE_ADD(length);MAZE_ADD(seen);MAZE_ADD(distinct);MAZE_ADD(coverage);
#undef MAZE_ADD
}
}
#define MAZE_ENTRY(t,l) ENTRY void maze##l##_##t(const Genome *g,const SynapticLayer *d,const AssociationLayer *a,const OutcomeLayer *m,const Word *live,Word n,Settings c,const Maze *w,Travel p,Word rooms,Word roam,TravelArm arm,MazeScore *r,Word first,Word count) { maze_life<t,l>(g,d,a,m,live,n,c,w,p,rooms,roam,arm,r,first,count); }
#define MAZE_LEVEL(t) MAZE_ENTRY(t,0) MAZE_ENTRY(t,1) MAZE_ENTRY(t,2) MAZE_ENTRY(t,3)
MAZE_LEVEL(2)
MAZE_LEVEL(4)
MAZE_LEVEL(8)
MAZE_LEVEL(16)
MAZE_LEVEL(32)
#undef MAZE_ENTRY
#undef MAZE_LEVEL
