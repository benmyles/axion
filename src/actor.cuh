#include "axion/actor_batch.hpp"
// Fold any binary sensory field through the same inherited address wiring used
// by physical receptor signatures. Each absent address wire aliases its halves.
namespace {
AX_FN Word project_spikes(Word input, Word connected) {
    if (connected == 31) return input;
    UNROLL
    for (int b = 0; b < 5; ++b) {
        Word distance = 1u << b;
        if (!((connected >> b) & 1)) {
            Word low = ~0u / ((1u << distance) + 1u);
            input = (input | (input >> distance)) & low;
        }
    }
    return input;
}
template <int Tiles, int Relational = 0, bool Balanced = false, bool Navigation = false, bool Series = false>
AX_FN void perform(Individual *actor, const Encounter *events, Response *answers, Word count,
                   AssociationLayer *links = nullptr, OutcomeLayer *organ = nullptr, Word perception = 0,
                   Word intervention = 0, Word types = 0, Word offset = 0, Word *summary = nullptr, bool frozen = false,
                   bool inherited_eligibility = false, bool motor_learning = true, Word expressed = 0) {
    Word tile=index()&(Tiles-1),base=(index()&31)&~(Tiles-1),mask=Word((1ull<<Tiles)-1)<<base;
    Word checksum=0,matches=0;
    const Genome &g = actor->genes;
    bool grove = actor->has_dendrites, fresh = !actor->ticks;
    Word start = actor->ticks, ticks = start, due = actor->due, motors = actor->motor_history;
    Word lag = actor->delayed_path ? conduction(g) : 1;
    auto *layer = actor->bits == 2 ? &actor->layer : nullptr;
    Tissue tissue = fresh ? develop(mix(g.development + tile*0x9e3779b9u)) : actor->tissue[tile];
    if (expressed && tile >= expressed)
        tissue = {};
    auto advance = [&](Tissue &state, Word sensory) {
        // Portable floodplain residents keep their lifetime-developed extent.
        if constexpr (!Relational && !Balanced && !Navigation) {
            if (expressed) {
                neural<Tiles, false, true>(g, state, sensory, tile, base, mask, true, expressed, expressed);
                return;
            }
        }
        neural<Tiles, false>(g, state, sensory, tile, base, mask, true);
    };
    Dendrites<Tiles,Relational> synapses;
    auto *expression=Relational==3 && actor->alternate_program ? expression_tail(links,1) : nullptr;
    synapses.load(g,&actor->contacts,tile,fresh,layer,links,Relational==3 ? context_tail(links,1) : nullptr,expression,actor->alternate_program==2);
    Prospects<Tiles> predictor{};
    MapCircuit<Tiles> map;
    Word previous_place=0;
    if constexpr (Navigation) {
        const auto &stored=map_tail(organ,1)[0];
        map.load(stored,tile,fresh);map.load_trace(organ,1,0,tile,fresh,perception);map.cut=MapCut(intervention); previous_place=fresh ? 0 : stored.previous;
        if (Series || frozen) map.frozen_graph(tile,base,mask);
    }
    Word last_event=0;
    if constexpr (Balanced) { predictor.load(*organ,tile,fresh); last_event=organ->associations&0xffff0000u; }
    auto credit=[&](Dendrites<Tiles,Relational> &state,Word signal) {
        if (!motor_learning || !signal || ticks<lag) return;
        // The first contact follows the original inherited conduction time.
        // Three optional delayed discharges use the same eight-event history.
        Word eligible=1;
        if (inherited_eligibility) eligible|=((g.reproduction>>20)&7)<<1;
        while (eligible) {
            Word age=lag+Word(__builtin_ctz(eligible));eligible&=eligible-1;
            if (age>8 || ticks<age) continue;
            Word old=!tile ? actor->afferent_history[(ticks-age)&7] : 0;
            old=shuffle(old,base,mask);
            Word projected=(Relational || Balanced) ? project_spikes(old,(g.edges[1]>>10)&31) : old;
            state.observe(g,g.expression>>16,old,projected,(motors>>(3*(age-1)))&7,signal==1,tile,true,age);
        }
    };
    // Frozen decisions start from the same moment in an animal's life.
    // Apply its already-due feedback once to the private register copy.
    if ((Series || frozen) && grove) credit(synapses,(due>>(2*(ticks&7)))&3);
    Word cursor=offset;
    for (Word i = 0; i < count; ++i) {
        Encounter e=events[Series ? cursor : i];
        if constexpr (Series) { e.kind=EventKind::orient;if (++cursor==types) cursor=0; }
        if constexpr (Navigation) if (e.kind==EventKind::savour) {
            if (ticks && tasted(last_event)) {
                Word previous=!tile ? actor->afferent_history[(ticks-1)&7] : 0;
                previous=shuffle(previous,base,mask);
                predictor.observe(g,previous,(last_event&(1u<<22)) ? 8 : motors&7,tasted(last_event),absorbed(last_event),tile);
            }
            map.associate(map.address(e.receptors),e.value,tile);map.sense_event(e.receptors,tile,base,mask);
            advance(tissue, e.receptors ^ food_field(0, 0, receipt(0, 0, e.value)));
            if (!tile) { answers[i]={8,0,e.value,0};actor->afferent_history[ticks&7]=0; }
            previous_place=0;last_event=0;synapses.remember(4);
            motors=(motors<<3)&0xffffffu;due&=~(3u<<(2*(ticks&7)));++ticks;continue;
        }
        bool stride=e.kind==EventKind::stride;
        bool travel=e.kind==EventKind::travel || e.kind==EventKind::orient || stride;
        bool probe=e.kind==EventKind::probe || e.kind==EventKind::seek || e.kind==EventKind::orient;
        bool food_event=e.kind==EventKind::sample || e.kind==EventKind::seek;
        bool passive=e.kind==EventKind::taste;
        Word shift = 2*(ticks & 7);
        Word signal = e.kind == EventKind::step ? e.signal : (due >> shift) & 3;
        Word raw = project_spikes(e.receptors, (g.edges[1] >> 10) & 31);
        Tissue next = tissue;
        auto learned = synapses;
        auto projection=predictor;
        Word route_need=stride ? e.mask : travel ? e.signal>>1 : 0;
        Word need=food_event ? e.signal : 0, field=e.ambient|(route_need<<20);
        if constexpr (Balanced) {
            if (ticks && tasted(last_event)) {
                Word previous=!tile ? actor->afferent_history[(ticks-1)&7] : 0;
                previous=shuffle(previous,base,mask);
                projection.observe(g,previous,(last_event&(1u<<22)) ? 8u : motors&7,tasted(last_event),absorbed(last_event),tile);
            }
            Word stimulus=passive ? receipt(0,e.mask,e.value) : last_event;
            if (food_event || passive) field=food_field(field,need,stimulus);
        }
        Word ambient=field;
        if (signal && !(food_event || passive)) field |= 1u << (signal == 1 ? 22 : 23);
        advance(next, field ^ e.receptors);
        Word tie = mix(actor->seed ^ ticks*0xc2b2ae35u);
        auto votes = motor_counts<Tiles, true>(g, next, tile, base, mask,
                                               expressed ? expressed : living_tiles(Tiles, g.development));
        Word drive = 0, stamp = 0;
        if (grove) {
            if (!Series && !frozen) credit(learned,signal);
            if (!passive) drive=learned.read(g,e.receptors,raw,tile,base,mask,false,nullptr,ambient,&stamp,&votes,tie);
        }
        if constexpr (Balanced) if (passive) {
            if constexpr (Navigation) previous_place=0;
            if (!tile) { answers[i]={8,0,signal,0}; actor->afferent_history[ticks&7]=0; }
            learned.remember(4); synapses=learned; tissue=next; predictor=projection;
            last_event=receipt(0,e.mask,e.value)|(1u<<22);
            motors=(motors<<3)&0xffffffu; due&=~(3u<<shift); ++ticks; continue;
        }
        if constexpr (Balanced) drive=projection.drive(g,e.receptors,need,drive,tile,base,mask);
        if constexpr (Navigation) if (travel) {
            if (stride) map.encounter(previous_place,e.receptors,e.value,tile);
            drive=map.drive(g,e.receptors,e.ambient,route_need,drive,tile,base,mask,perception,tie,&votes);
        }
        Word action=grove ? select_motor<true>(g,votes,tie,drive) : select_motor<false>(g,votes,tie);
        Word ports = effector(g, action);
        bool matched = (ports & e.mask) == e.value;
        Word flavour=0,food=0;
        if constexpr (Balanced) if (food_event) {
            flavour=(ports&8) ? 1+((e.mask>>(2*(ports&3)))&3) : 0;
            if (flavour && e.value<256) food=1+((e.value>>(2*(flavour-1)))&3);
            matched=(food_spike(food)&need)!=0;
        }
        if constexpr (Navigation) if (!probe) map.sense_event(e.receptors,tile,base,mask);
        Word arrival=0;
        if (e.kind==EventKind::travel) {
            arrival=(ports&e.value) ? ((e.mask>>(8*(ports&3)))&31) : Word(__builtin_ctz(e.receptors));
            if constexpr (Navigation) map.encounter(map.note(e.receptors,ports),1u<<arrival,0,tile);
            advance(next, 1u << arrival);
            if constexpr (Navigation) map.sense_event(1u<<arrival,tile,base,mask);
        }
        if constexpr (Series) { matches+=matched;checksum=mix(checksum^ports^i); }
        else if (!tile) {
            Word report=Word(matched);
            if (e.kind==EventKind::step && e.mask==vocal_margin_report) {
                // Measure the same eight populations that issued the command.
                // Tied noise chooses a muscle but contributes no evidence.
                Word winner=0,rival=0;
                UNROLL
                for (Word k=0;k<8;++k) {
                    Word bank=k^(g.motors>>29);
                    Word activity=((bank<4 ? votes.low : votes.high)>>(8*(bank&3)))&255;
                    if (grove) activity+=((drive>>(4*k))&15)*votes.gain;
                    if (k==action) winner=activity;
                    if ((effector(g,k)&11)!=(ports&11) && activity>rival) rival=activity;
                }
                report=winner>rival ? winner-rival : 0;
            }
            answers[i]={action,ports,e.kind==EventKind::travel ? arrival : signal|(flavour<<2)|(food<<5),report};
        }
        if (probe) continue;
        if constexpr (Navigation) previous_place=stride ? map.note(e.receptors,ports) : 0;
        if constexpr (Balanced) { predictor=projection; last_event=receipt(0,flavour,food); }
        learned.remember(stamp);
        tissue = next; synapses = learned;
        if (!tile) actor->afferent_history[ticks & 7] = (Relational || Balanced) ? e.receptors : raw;
        motors = ((motors << 3) | action) & 0xffffffu;
        due &= ~(3u << shift);
        if (e.kind==EventKind::trial || (food_event && e.value<256)) due |= (matched ? 1u : 2u) << (2*((ticks + actor->outcome_lag) & 7));
        ++ticks;
    }
    if constexpr (Series) { if (!tile) { summary[0]=matches;summary[1]=checksum; } return; }
    if (ticks == start) return;
    actor->tissue[tile] = tissue;
    if constexpr (Navigation) {
        auto &storage=map_tail(organ,1)[0];
        map.store(storage,tile); map.store_trace(organ,1,0,tile);if (!tile) storage.previous=previous_place;
    }
    if (grove) synapses.store(actor->contacts, tile, layer, links);
    if constexpr (Relational==3) synapses.store_context(*context_tail(links,1),tile,expression);
    if constexpr (Balanced) {
        predictor.store(*organ,tile);
        if (!tile) organ->associations=(organ->associations&0xff00ffffu)|(last_event&0x00ff0000u);
    }
    if (!tile) { actor->ticks = ticks; actor->due = due; actor->motor_history = motors; }
}
template<int Tiles,int Level,bool Balanced=false,bool Navigation=false>
AX_FN void actor_group(Individual *brains,const Encounter *events,Response *answers,Word count,AssociationLayer *paths,
                       OutcomeLayer *organs,Word mode,Word cut,Word frozen,ActorBatch layout) {
    Word id=index()/Tiles;
    if (id>=layout.copies) return;
    auto *body=reinterpret_cast<Individual *>(reinterpret_cast<unsigned char *>(brains)+Count(id)*layout.body_bytes);
    if constexpr (Level) paths=reinterpret_cast<AssociationLayer *>(reinterpret_cast<unsigned char *>(paths)+Count(id)*layout.path_bytes);
    if constexpr (Balanced) organs=reinterpret_cast<OutcomeLayer *>(reinterpret_cast<unsigned char *>(organs)+Count(id)*layout.organ_bytes);
    perform<Tiles, Level, Balanced, Navigation>(body, events + Count(id) * count, answers + Count(id) * count, count,
                                                paths, organs, mode, cut, 0, 0, nullptr, (frozen & 1) != 0,
                                                layout.eligibility != 0, layout.motor_learning != 0, frozen >> 1);
}
} // namespace
#define ACTOR_ARGS Individual *a,const Encounter *e,Response *r,Word count,AssociationLayer *p,OutcomeLayer *m,Word mode,Word cut,Word frozen,ActorBatch layout
#define ACTOR_PARAMS a,e,r,count,p,m,mode,cut,frozen,layout
#if defined(AX_FAMILY_actors)
#define REL_ACT(n,prefix,level) ENTRY void prefix##_actor##n(ACTOR_ARGS) { actor_group<n,level>(ACTOR_PARAMS); }
#define ACT(n) ENTRY void actor##n(ACTOR_ARGS) { actor_group<n,0>(ACTOR_PARAMS); } \
    REL_ACT(n,linked,1) REL_ACT(n,wave,2) REL_ACT(n,context,3)
ACT(2) ACT(4) ACT(8) ACT(16) ACT(32)
#undef ACT
#undef REL_ACT
#else
#ifdef AX_FAMILY_mapactors
#define MOBILE true
#else
#define MOBILE false
#endif
#define FOOD_ACT(n,l) ENTRY void food##l##_actor##n(ACTOR_ARGS) { actor_group<n,l,true,MOBILE>(ACTOR_PARAMS); }
#define FOOD_ACTORS(n) FOOD_ACT(n,0) FOOD_ACT(n,1) FOOD_ACT(n,2) FOOD_ACT(n,3)
FOOD_ACTORS(2) FOOD_ACTORS(4) FOOD_ACTORS(8) FOOD_ACTORS(16) FOOD_ACTORS(32)
#undef FOOD_ACTORS
#undef FOOD_ACT
#endif
#undef ACTOR_PARAMS
#undef ACTOR_ARGS

#if defined(AX_FAMILY_mapactors)
ENTRY void replicate_words(const Word *source,Word *dest,Word words,Word copies) {
    Word i=index(); if (i<words*copies) dest[i]=source[i%words];
}
namespace {
template<int Tiles,int Level> AX_FN void frozen_series(unsigned char *brains,unsigned char *links,unsigned char *organs,
       const Encounter *queries,Word *out,Word copies,Word count,Word kinds,Word body,Word synapses,Word mode,Word cut) {
    Word id=index()/Tiles;
    if (id>=copies) return;
    auto *animal=reinterpret_cast<Individual *>(brains+Count(id)*body);
    auto *cortex=Level ? reinterpret_cast<AssociationLayer *>(links+Count(id)*synapses) : nullptr;
    auto *organ=reinterpret_cast<OutcomeLayer *>(organs+Count(id)*(sizeof(OutcomeLayer)+map_bytes(mode)));
    perform<Tiles,Level,true,true,true>(animal,queries,nullptr,count,cortex,organ,mode,cut,kinds,mix(id)%kinds,out+2*id);
}
}
#define SERIES(t,l) ENTRY void bearings##l##_##t(unsigned char *a,unsigned char *s,unsigned char *p,const Encounter *q,Word *r,Word n,Word count,Word kinds,Word body,Word synapses,Word mode,Word cut) { frozen_series<t,l>(a,s,p,q,r,n,count,kinds,body,synapses,mode,cut); }
#define SERIES_LEVEL(t) SERIES(t,0) SERIES(t,1) SERIES(t,2) SERIES(t,3)
SERIES_LEVEL(2)
SERIES_LEVEL(4)
SERIES_LEVEL(8)
SERIES_LEVEL(16)
SERIES_LEVEL(32)
#undef SERIES
#undef SERIES_LEVEL
#endif
