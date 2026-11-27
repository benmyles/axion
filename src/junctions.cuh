#include "axion/passage.hpp"
namespace {
enum class PassageArm : Word { intact,raw,fixed,brief,no_hold,hidden,swapped,labile,no_novelty,fixed_support };
struct SimilarRooms {
    CrossingWorld landmarks;
    Word actions,wings,outcomes,meals,shared,rotation,places;
    AX_FN Word marker(Word w) const { return landmarks.cue(w); }
    AX_FN Word room(Word w,Word station) const { return landmarks.cue(shared ? (station ? 26+station : 8u) : 18+4*station+w); }
    AX_FN Word end(Word w,Word muscle,Word epoch,Word station) const {
        Word key=landmarks.key^station*0xc1d7936bu;
        Word a=station ? permutation(mix(key+7),0) : actions;
        Word b=station ? permutation(mix(key+11),0) : wings;
        Word c=station ? permutation(mix(key+13),0) : outcomes;
        Word action=(a>>(2*((muscle+rotation)&3)))&3,wing=(b>>(2*w))&3;
        return (c>>(2*((action+wing+epoch)&3)))&3;
    }
    AX_FN Word food(Word k) const { return (meals>>(2*k))&3; }
};

template<int Tiles,int Level> struct PassageAnimal {
    Traveller<Tiles,Level> a;
    Word pending=0,ticks=0,seed=0;
    bool lifetime_clock=true;
    AX_FN Word pace(Word draw) const { return lifetime_clock ? mix(seed^ticks*0xc2b2ae35u) : draw; }
    AX_FN void pass(Word tag,Word time,bool writing) {
        Word raw=1u<<tag,stamp=0;
        if (writing && a.c.navigation && a.c.plastic) a.map.encounter(pending,raw,0,a.tile);
        Word pins=a.choose(a.soma,tag,0,pace(time),&stamp);++ticks;
        a.banks.remember(stamp);
        if (a.c.navigation) { a.map.sense_event(raw,a.tile,a.base,a.mask);pending=a.map.note(raw,pins); }
    }
    AX_FN void approach(const SimilarRooms &w,Word wing,Word latency,Word noise,Word key,bool write,PassageArm arm) {
        Word marker=arm==PassageArm::hidden ? w.landmarks.cue(9) : w.marker((wing+Word(arm==PassageArm::swapped))&3);
        pass(marker,mix(key+1),write);
        for (Word t=0;t<latency;++t) {
            Word draw=mix(key^t*0x632be59bu^0xd9437cb1u);
            Word texture=noise ? 10+(draw&7) : 9;
            if (noise==2) {
                Word first=w.shared ? 18 : 18+4*w.places;
                texture=first+draw%(w.shared ? 8 : 32-first);
            }
            Word tag=w.landmarks.cue(texture);
            pass(tag,draw,write);
        }
    }
    __attribute__((device,noinline)) Word probe(const SimilarRooms &w,JunctionTrial cfg,Word changed,PassageArm arm,Word key) {
        Word score=0;
        auto subject=*this;
        Word order=permutation(mix(key^0x3eb92d1fu),0);
        for (Word n=0;n<4;++n) {
            Word wing=(order>>(2*n))&3;
            if (cfg.traversal!=1 && n) {
                subject.a.map=a.map;subject.a.banks=a.banks;subject.a.soma=a.soma;subject.pending=pending;subject.ticks=ticks;
            }
            Word noise=cfg.traversal==1 ? mix(key+n*0x9ec59767u) : key;
            subject.approach(w,wing,cfg.delay,cfg.noise,noise,cfg.traversal!=2,arm);
            for (Word station=0;station<cfg.places;++station) for (Word goal=0;goal<4;++goal) {
                Tissue copy=subject.a.soma;
                Word pins=subject.a.choose(copy,w.room(wing,station),cfg.motivation ? 0 : 1u<<w.landmarks.cue(4+goal),
                                           subject.pace(mix(key+goal*0xbb72f19du)),nullptr,cfg.motivation ? 1u<<w.food(goal) : 0);
                score+=Word((pins&8) && w.end(wing,pins&3,changed,station)==goal);
            }
        }
        if (cfg.traversal==1) { a.map=subject.a.map;a.banks=subject.a.banks;a.soma=subject.a.soma;pending=subject.pending;ticks=subject.ticks; }
        return score;
    }
    __attribute__((device,noinline)) Word explore(const SimilarRooms &w,JunctionTrial cfg,Word changed,Word key,Word &moved) {
        Word covered[3]{};
        for (Word round=0;round<cfg.rounds;++round) {
            Word order=permutation(mix(key^round*0x7e4f15cbu),0);
            for (Word i=0;i<4*cfg.places;++i) {
                Word wing=(order>>(2*(i&3)))&3,station=(i/4+wing+round)%cfg.places;
                Word draw=mix(key^(round*4*cfg.places+i)*0x9b1d038bu);
                Word delay=cfg.variable ? mix(draw+3)%(cfg.practice_delay+1) : cfg.practice_delay;
                approach(w,wing,delay,cfg.practice_noise,draw,true,PassageArm::intact);
                Word tag=w.room(wing,station),raw=1u<<tag,stamp=0;
                Word pins=a.choose(a.soma,tag,0,pace(mix(draw^0xc2a43d97u)),&stamp);++ticks;
                Word target=pins&8 ? w.landmarks.cue(4+w.end(wing,pins&3,changed,station)) : tag;
                a.banks.remember(stamp);
                if (a.c.navigation) {
                    a.map.sense_event(raw,a.tile,a.base,a.mask);
                    if (a.c.plastic) a.map.encounter(a.map.note(raw,pins),1u<<target,0,a.tile);
                    a.map.sense_event(1u<<target,a.tile,a.base,a.mask);
                }
                neural<Tiles,false>(a.g,a.soma,1u<<target,a.tile,a.base,a.mask,a.c.plastic);
                pending=0;
                if (pins&8) { ++moved;covered[station]|=1u<<(4*wing+(pins&3)); }
            }
        }
        return population(covered[0])+population(covered[1])+population(covered[2]);
    }
};
template<int Tiles,int Level,bool Breed>
AX_FN void passage_life(const Genome *genes,const SynapticLayer *layers,const AssociationLayer *links,const OutcomeLayer *organs,
                        const Word *living,Word survivors,Settings c,JunctionTrial cfg,PassageArm arm,PassageScore *totals,Aptitude *fitness,Word first,Word count) {
    Word id=index(),member=id/Tiles,tile=id&(Tiles-1);
    if (member>=count) return;
    Word episode=first+member,group=episode/4,slot=0;
    Word candidate=0;
    if constexpr (Breed) { candidate=group/cfg.sets;slot=living ? living[candidate] : candidate;group%=cfg.sets; }
    Word key=mix(cfg.seed^group*0xa341316cu);
    // Vary the sensory environment across worlds. All four rotations and
    // their counterfactual partners receive the same exposure schedule.
    if (cfg.traversal==3) cfg.traversal=(mix(key^0x7afe1c8du)>>17)&1;
    if (cfg.noise==3) cfg.noise=1+((mix(key^0x2db93a17u)>>13)&1);
    if constexpr (!Breed) slot=living[mix(key^0xa4d2e54bu)%survivors];
    Word base=(id&31)&~(Tiles-1),mask=Word((1ull<<Tiles)-1)<<base,sites=c.side*c.side;
    const auto &g=genes[slot];
    PassageAnimal<Tiles,Level> pupil{{g,develop(mix(g.development+tile*0x9e3779b9u)),{},{},tile,base,mask,c}};
    pupil.seed=mix(c.seed^slot);pupil.lifetime_clock=cfg.lifetime_clock;
    auto &a=pupil.a;
    a.banks.load(g,nullptr,tile,true,layers ? layers+slot : nullptr,Level ? links+slot : nullptr,
                 Level==3 ? context_tail(links,sites)+slot : nullptr,Level==3 && c.programs==2 ? expression_tail(links,sites)+slot : nullptr,c.polarity);
    if (c.navigation) {
        a.map.load(map_tail(organs,sites)[slot],tile,true);
        a.map.load_trace(organs,sites,slot,tile,true,c.navigation);
        if (arm==PassageArm::fixed) a.map.cut=MapCut::trace_fixed;
        if (arm==PassageArm::labile) a.map.cut=MapCut::labile;
        if (arm==PassageArm::no_novelty) a.map.cut=MapCut::no_novelty;
        if (arm==PassageArm::fixed_support) a.map.cut=MapCut::fixed_support;
    }
    SimilarRooms world{{key,0,0},permutation(mix(key+7),0),permutation(mix(key+11),0),permutation(mix(key+13),0),permutation(mix(key+17),0),Word(!cfg.visible),episode&3,cfg.places};
    if (cfg.motivation==2) cfg.motivation=(key>>21)&1;
    if (cfg.motivation) for (Word f=0;f<4;++f) {
        a.taste(world.landmarks.cue(4+f),1+world.food(f));a.banks.remember(4);++pupil.ticks;
    }
    Word moved=0,naive=0,surprise=0,covered=0;
    auto probe=[&](Word epoch) {
        auto prior=a.map.cut;
        if (arm==PassageArm::raw) a.map.cut=MapCut::raw;
        if (arm==PassageArm::brief) a.map.cut=MapCut::immediate;
        if (arm==PassageArm::no_hold) a.map.cut=MapCut::no_hold;
        Word result=pupil.probe(world,cfg,epoch,arm,mix(key^0x7adba189u));
        a.map.cut=prior;return result;
    };
    if (!Breed || cfg.traversal==1) naive=probe(0);
    covered+=pupil.explore(world,cfg,0,mix(key+19),moved);
    Word acquired=probe(0);
    if (!Breed || cfg.traversal==1) surprise=probe(1);
    covered+=pupil.explore(world,cfg,1,mix(key+23),moved);
    Word revised=probe(1);
    if (tile) return;
    if constexpr (Breed) {
        auto *out=fitness[candidate].score+cfg.offset;
        asm volatile("red.global.add.u32 [%0],%1;" :: "l"(out),"r"(acquired*8):"memory");
        asm volatile("red.global.add.u32 [%0],%1;" :: "l"(out+1),"r"(revised*8):"memory");
    } else {
        auto &v=totals[Word(arm)];
#define PASSAGE_ADD(f,n) asm volatile("red.global.add.u64 [%0],%1;" :: "l"(&v.f),"l"(Count(n)):"memory")
        PASSAGE_ADD(naive,naive);PASSAGE_ADD(acquired,acquired);PASSAGE_ADD(surprise,surprise);PASSAGE_ADD(revised,revised);
        PASSAGE_ADD(observed,8*cfg.rounds*cfg.places);PASSAGE_ADD(moved,moved);PASSAGE_ADD(covered,covered);
#undef PASSAGE_ADD
    }
}
}
#define PASSAGE_ARGS const Genome *g,const SynapticLayer *d,const AssociationLayer *a,const OutcomeLayer *m,const Word *live,Word n,Settings c,JunctionTrial q,PassageArm arm,PassageScore *r,Aptitude *f,Word first,Word count
#define PASSAGE_KERNEL(t,l) \
ENTRY void junction##l##_##t(PASSAGE_ARGS) { passage_life<t,l,false>(g,d,a,m,live,n,c,q,arm,r,f,first,count); } \
ENTRY void waymark##l##_##t(PASSAGE_ARGS) { passage_life<t,l,true>(g,d,a,m,live,n,c,q,arm,r,f,first,count); }
#define PASSAGE_LEVELS(t) PASSAGE_KERNEL(t,0) PASSAGE_KERNEL(t,1) PASSAGE_KERNEL(t,2) PASSAGE_KERNEL(t,3)
PASSAGE_LEVELS(2) PASSAGE_LEVELS(4) PASSAGE_LEVELS(8) PASSAGE_LEVELS(16) PASSAGE_LEVELS(32)
#undef PASSAGE_LEVELS
#undef PASSAGE_KERNEL
#undef PASSAGE_ARGS
