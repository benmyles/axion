// The hidden contingency exists exclusively in the environment. Returning and
// staying start from bit-identical copies of the same uninterrupted lifetime.
namespace {
struct ReturningWorld {
    Revisit cfg;
    Word key, twist;
    AX_FN bool mixed() const { return cfg.task==RevisitTask::interleaved; }
    AX_FN bool pair(Word item) const { return cfg.task==RevisitTask::paired || (mixed() && item>=cfg.items); }
    AX_FN Word anchors() const { return cfg.task==RevisitTask::arbitrary || mixed() ? 2 : 4; }
    AX_FN Word exceptions() const { return cfg.task==RevisitTask::exceptions ? cfg.items/4 : 0; }
    AX_FN bool exception(Word item) const { return item>=4 && item<4+exceptions(); }
    AX_FN Word test_count() const { return recall_items(cfg); }
    AX_FN Word domain_score(Word correct,Word unary) const {
        Word a=256*unary/(cfg.items-2), b=256*(correct-unary)/(cfg.items*cfg.items/2);
        return (a+b+2*lesser(a,b))/4;
    }
    AX_FN Word held_out(Word i) const {
        if (mixed()) {
            if (i<cfg.items-2) return 2+i;
            i-=cfg.items-2;
        } else if (cfg.task!=RevisitTask::paired) return anchors()+i;
        Word shape=i&(cfg.items-1), offset=cfg.items/2+i/cfg.items;
        return (mixed() ? cfg.items : 0)+shape+cfg.items*((shape+offset)&(cfg.items-1));
    }
    AX_FN Word training_size(Word regime) const {
        if (mixed()) return regime ? cfg.items+4 : 2*cfg.items;
        if (regime && cfg.task!=RevisitTask::arbitrary) return anchors()+exceptions();
        return cfg.task==RevisitTask::paired ? cfg.items*cfg.items/2 : cfg.items;
    }
    AX_FN Word exemplar(Word i) const { return cfg.task==RevisitTask::paired ? cfg.items*i : i; }
    AX_FN Word shuffled(Word regime,Word step,Word n,Word salt=0) const {
        Word shuffle_key=mix(key ^ (step/n)*0x8538a5e5u ^ regime*0xaf7239cbu ^ salt);
        return (n&(n-1)) ? ((shuffle_key>>16)+((shuffle_key&1) ? step%n : n-1-step%n))%n
                          : ((shuffle_key>>16)+(shuffle_key|1)*step)&(n-1);
    }
    AX_FN Word practice_item(Word regime, Word step) const {
        Word index;
        if (mixed() && !regime) {
            // Every successive pair of trials has one object from each domain.
            // The independent permutations cover both complete practice sets.
            Word round=step/2;
            if (!((step^mix(key+round))&1)) return shuffled(0,round,cfg.items);
            index=shuffled(0,round,cfg.items*cfg.items/2,0x34817ae5u);
        } else {
            index=shuffled(regime,step,training_size(regime));
            if (mixed()) return index<cfg.items ? index : cfg.items*(1+index-cfg.items);
        }
        if (mixed()) {
            Word shape=index&(cfg.items-1);
            return cfg.items+shape+cfg.items*((shape+index/cfg.items)&(cfg.items-1));
        }
        if (regime && cfg.task!=RevisitTask::arbitrary) return exemplar(index);
        if (cfg.task!=RevisitTask::paired) return index;
        Word shape=index&(cfg.items-1);
        return shape+cfg.items*((shape+index/cfg.items)&(cfg.items-1));
    }
    AX_FN Word cue(Word item) const {
        if (item==~0u) return panel_tag(key,cfg.items);
        if (!pair(item)) return panel_tag(key,item);
        if (mixed()) item-=cfg.items;
        return PanelItem{item&(cfg.items-1),item/cfg.items}.cue(key,true);
    }
    AX_FN Word affordance(Word item) const {
        if (!pair(item))
            return (permutation(mix(key^0xcd9def57u),item>>2)>>(2*(item&3)))&3;
        if (mixed()) item-=cfg.items;
        Word s=item&(cfg.items-1), t=item/cfg.items;
        Word a=permutation(mix(key^0x14d9cfabu),s>>2)>>(2*(s&3));
        Word b=permutation(mix(key^0xec0b2059u),t>>2)>>(2*(t&3));
        return (a+b)&3;
    }
    AX_FN Word target(Word regime, Word item) const {
        if (item==~0u) return (key+twist)&3;
        if (cfg.task==RevisitTask::arbitrary || (mixed() && !pair(item))) {
            Word table=permutation(mix(key ^ (item*0xb5297a4du) ^ 0x308fc5c9u),0);
            return ((table>>(2*regime))+twist)&3;
        }
        Word frame_id=regime;
        if (regime && exception(item)) {
            Word draw=mix(key ^ (item*0xfba3a61du));
            frame_id=cfg.regimes==2 ? 2+(draw&1) : 1+(regime+draw%(cfg.regimes-2))%(cfg.regimes-1);
        }
        Word frame=(permutation(mix(key^0x89eb6527u),0)>>(2*frame_id))&3;
        Word decoder=permutation(mix(key^0x52afe733u),0);
        return ((decoder>>(2*((affordance(item)+frame)&3)))+twist)&3;
    }
    AX_FN Word ambient(Word regime) const {
        return cfg.marked ? mix(key ^ (regime*0x51ed272du) ^ 0xf37bb5f9u) : 0x01070000u;
    }
};

template<int Tiles,int Level>
struct Witness {
    Tissue state;
    Dendrites<Tiles,Level> synapses;
    PeripheralHistory history{};
    Word time=0, recent=0, outcomes=0, regime_history=0;
    Word alternate_trials=0;
    AX_FN Word choose(const Genome &g, Word cue, Word field, Word tie, Word tile, Word base, Word mask, Settings c) const {
        Tissue copy=state;
        return take_action<Tiles>(g,copy,synapses,cue,0,tie,tile,base,mask,c,false,nullptr,field);
    }
    AX_FN Word encounter(const Genome &g, const ReturningWorld &world, Word regime, Word field, Word item, Word tie,
                         Word tile, Word base, Word mask, Settings c, bool locked, bool &success) {
        Word cue=world.cue(item), stamp=0;
        if constexpr (Level==3) if (synapses.forced_group<4) synapses.forced_group=regime;
        Settings physical=c;
        if (locked) physical.plastic=0;
        Word motor=take_action<Tiles>(g,state,synapses,cue,recent,tie,tile,base,mask,physical,false,nullptr,field,&stamp);
        if constexpr (Level==3) alternate_trials+=(stamp>>10)&1;
        success=consumes(g,motor,world.target(regime,item));
        synapses.remember(stamp); history.append(delayed(cue,motor));
        outcomes=(outcomes<<1)|Word(success); ++time;
        Word lag=c.digestion ? conduction(g) : 1, delay=world.cfg.delay;
        if constexpr (Level==3) if (synapses.forced_group<4) regime_history=(regime_history<<2)|regime;
        recent=0;
        if (time>=delay) {
            bool signal=(outcomes>>(delay-1))&1;
            recent=signal ? 1 : 2;
            if (c.plastic && c.habitat!=Habitat::meadow && time>=lag) {
                Word old=history.at(lag);
                if constexpr (Level==3) if (synapses.forced_group<4) synapses.forced_group=(regime_history>>(2*(lag-1)))&3;
                if (!locked) synapses.update(g,g.expression>>16,old,signal,tile,true,lag);
                else if constexpr (Level==3) {
                    synapses.credit_program(lag);
                    synapses.experience(g,pattern(earlier_cue(old)),(old>>5)&7,signal,lag,tile);
                }
            }
        }
        if constexpr (Level==3) if (synapses.forced_group<4) synapses.forced_group=regime;
        return motor;
    }
    AX_FN Word probe(const Genome &g, const ReturningWorld &world, Word regime, Word field, Word tile,
                     Word base, Word mask, Settings c,Word *subset=nullptr) const {
        Word correct=0;
        for (Word i=0;i<world.test_count();++i) {
            Word item=world.held_out(i);
            Word motor=choose(g,world.cue(item),field,mix(world.key ^ (i*0x1b873593u)),tile,base,mask,c);
            bool matched=consumes(g,motor,world.target(regime,item));
            correct+=matched;
            if (subset && (world.mixed() ? !world.pair(item) : world.exception(item))) *subset+=matched;
        }
        return correct;
    }
};
template<int Tiles,int Level,bool Select>
AX_FN void revisit(const Genome *genes,const SynapticLayer *layers,const AssociationLayer *links,const Word *live,Word survivors,
                   RevisitScore *totals,Aptitude *fitness,Settings c,Revisit cfg,RevisitArm arm,Word first,Word count) {
    Word id=index(), in_batch=id/Tiles, tile=id&(Tiles-1);
    if (in_batch>=count) return;
    Word trial=first+in_batch, group=trial/4, slot=0;
    if constexpr (Select) { slot=group/cfg.sets; group%=cfg.sets; }
    Word key=mix(cfg.seed ^ group*0x9e3779b9u);
    if constexpr (!Select) slot=live[mix(key^0x29ba1c8du)%survivors];
    Word base=(id&31)&~(Tiles-1), mask=Word((1ull<<Tiles)-1)<<base;
    const Genome &g=genes[slot];
    ReturningWorld world{cfg,key,trial&3};
    Witness<Tiles,Level> animal;
    animal.state=develop(mix(g.development+tile*0x9e3779b9u));
    animal.synapses.load(g,nullptr,tile,true,layers ? &layers[slot] : nullptr,Level ? &links[slot] : nullptr,
                         Level==3 ? context_tail(links,c.side*c.side)+slot : nullptr,
                         Level==3 && c.programs==2 ? expression_tail(links,c.side*c.side)+slot : nullptr,c.polarity);
    if constexpr (Level==3) {
        if (arm==RevisitArm::pooled) animal.synapses.context_cut=ContextCut::pooled;
        if (arm==RevisitArm::blind) animal.synapses.context_cut=ContextCut::blind;
        if (arm==RevisitArm::no_binding) animal.synapses.context_cut=ContextCut::no_binding;
        if (arm==RevisitArm::no_reinstatement) animal.synapses.context_cut=ContextCut::no_reinstatement;
        if (arm==RevisitArm::fixed_confidence) animal.synapses.context_cut=ContextCut::fixed_confidence;
        if (arm==RevisitArm::labile) animal.synapses.context_cut=ContextCut::labile;
        if (arm==RevisitArm::external_only) animal.synapses.context_cut=ContextCut::external_only;
        if (arm==RevisitArm::assigned) animal.synapses.forced_group=0;
        if (arm==RevisitArm::primary_only) animal.synapses.context_cut=ContextCut::primary_only;
        if (arm==RevisitArm::alternate_only) animal.synapses.context_cut=ContextCut::alternate_only;
    }
    Word tested=world.test_count(), scale=Select && world.mixed() ? 256 : tested;
    Word acquired_min=scale, recall_min=scale, recall_sum=0;
    RecallCounts<Word> result{};
    if constexpr (!Select) for (Word goal=0;goal<4;++goal) {
        bool available=false;
        UNROLL
        for (Word m=0;m<8;++m) available |= consumes(g,m,goal);
        result.reach+=available;
    }
    for (Word regime=0;regime<cfg.regimes;++regime) {
        Word reps=cfg.practice + (mix(key+regime*0xacb46b91u) % (1+cfg.practice/4));
        Word n=(world.mixed() || (regime && cfg.task!=RevisitTask::arbitrary) ? world.training_size(regime) : cfg.items)*reps;
        Word field=world.ambient(regime);
        for (Word t=0;t<n;++t) {
            Word item=world.practice_item(regime,t);
            if ((t+(key&15))%17==0) item=~0u;
            bool success;
            animal.encounter(g,world,regime,field,item,mix(key ^ animal.time*0x632bd34bu),tile,base,mask,c,false,success);
            result.positive+=success; ++result.trained;
        }
        Word acquired_subset=0;
        Word learned=animal.probe(g,world,regime,field,tile,base,mask,c,world.mixed() ? &acquired_subset : nullptr);
        if constexpr (Select) if (world.mixed()) learned=world.domain_score(learned,acquired_subset);
        acquired_min=lesser(acquired_min,learned); result.acquired+=learned;
        if constexpr (!Select) {
            result.acquired_in[regime]+=learned;
            result.acquired_subset[regime]+=acquired_subset;
            for (Word a=0;a<world.anchors();++a) {
                Word item=world.exemplar(a);
                Word m=animal.choose(g,world.cue(item),field,mix(key^a),tile,base,mask,c);
                result.anchor_correct+=consumes(g,m,world.target(regime,item)); ++result.anchor_items;
            }
            if (regime==0 || regime+1==cfg.regimes) {
                ContextCut prior=ContextCut::intact;
                if constexpr (Level==3) { prior=animal.synapses.context_cut; animal.synapses.context_cut=ContextCut::silent; }
                Word base_correct=animal.probe(g,world,0,world.ambient(0),tile,base,mask,c);
                if (!regime) result.base_first+=base_correct; else result.base_final+=base_correct;
                Word anchor=0;
                for (Word a=0;a<world.anchors();++a) {
                    Word item=world.exemplar(a);
                    Word m=animal.choose(g,world.cue(item),world.ambient(0),mix(key^a),tile,base,mask,c);
                    anchor+=consumes(g,m,world.target(0,item));
                }
                if (!regime) result.base_anchor_first+=anchor; else result.base_anchor_final+=anchor;
                if constexpr (Level==3) animal.synapses.context_cut=prior;
            }
        }
    }
    if constexpr (Level==3) { result.used=population((animal.synapses.activity>>22)&15); result.alternate_trials=animal.alternate_trials; }
    if (arm==RevisitArm::erased) animal.synapses.develop(g,tile);
    if constexpr (Level==3) {
        if (arm==RevisitArm::fixed) animal.synapses.context_cut=ContextCut::fixed;
    }
    for (Word regime=0;regime<cfg.regimes;++regime) {
        auto branch=animal;
        if constexpr (Level==3) if (arm==RevisitArm::assigned) branch.synapses.forced_group=regime;
        Word label=arm==RevisitArm::wrong_cue ? (regime+1)%cfg.regimes : regime;
        Word field=world.ambient(label), possible=(1u<<cfg.regimes)-1, mask_history=0;
        Word previous=0;
        for (Word stage=0;stage<(Select ? 1u : recall_stages);++stage) {
            Word dose=Select ? 8 : recall_dose(stage);
            for (Word t=previous;t<dose;++t) {
                Word which=((key>>20)+(key|1)*t)&(world.anchors()-1);
                Word item=arm==RevisitArm::ambiguous ? ~0u : world.exemplar(which);
                if (arm==RevisitArm::paired_hint) item=cfg.items*(1+(((key>>20)+(key|1)*t)&3));
                bool received;
                Word motor=branch.encounter(g,world,regime,field,item,mix(key^0xef8365d9u^t*0xb5297a4du),
                                            tile,base,mask,c,arm==RevisitArm::fixed,received);
                if constexpr (!Select) {
                    Word compatible=0;
                    for (Word candidate=0;candidate<cfg.regimes;++candidate)
                        compatible |= Word(consumes(g,motor,world.target(candidate,item))==received)<<candidate;
                    mask_history=(mask_history<<4)|compatible;
                    if (t+1>=cfg.delay) possible &= (mask_history>>(4*(cfg.delay-1)))&15;
                }
            }
            previous=dose;
            if constexpr (Level==3) if (arm==RevisitArm::silent) branch.synapses.context_cut=ContextCut::silent;
            if constexpr (Level==3) if (arm==RevisitArm::swapped) branch.synapses.context_cut=ContextCut::swapped;
            Word subset=0;
            bool detail=world.mixed() || (regime && cfg.task==RevisitTask::exceptions);
            Word correct=branch.probe(g,world,regime,field,tile,base,mask,c,(world.mixed() || (!Select && detail)) ? &subset : nullptr);
            if constexpr (Select) if (world.mixed()) correct=world.domain_score(correct,subset);
            if constexpr (Level==3) if (arm==RevisitArm::silent || arm==RevisitArm::swapped) branch.synapses.context_cut=ContextCut::intact;
            if constexpr (Select) { recall_min=lesser(recall_min,correct); recall_sum+=correct; }
            else {
                auto &s=result.stage[stage];
                s.correct+=correct; if (regime+1<cfg.regimes) s.returning+=correct;
                s.by_regime[regime]+=correct;
                if (detail) { s.exceptions+=subset; s.transfer+=correct-subset; }
                Word compatible=population(possible);
                if (compatible==1) { s.identified+=tested; s.identified_correct+=correct; }
                s.oracle_units+=tested*(12/compatible);
            }
        }
    }
    if (tile) return;
    if constexpr (Select) {
        auto *s=fitness[slot].score+cfg.fitness_offset;
        Word denom=scale*cfg.regimes;
        Word acquire=64*(Word(result.acquired)+acquired_min*cfg.regimes)/denom;
        Word remember=64*(recall_sum+recall_min*cfg.regimes)/denom;
        asm volatile("red.global.add.u32 [%0], %1;" :: "l"(s),"r"(acquire):"memory");
        asm volatile("red.global.add.u32 [%0], %1;" :: "l"(s+1),"r"(remember):"memory");
    } else {
        auto &out=totals[Word(arm)];
#define ADD_RECALL(dst,src) asm volatile("red.global.add.u64 [%0], %1;" :: "l"(&(dst)),"l"(Count(src)):"memory")
        ADD_RECALL(out.acquired,result.acquired); ADD_RECALL(out.trained,result.trained);
        ADD_RECALL(out.positive,result.positive); ADD_RECALL(out.used,result.used);
        ADD_RECALL(out.alternate_trials,result.alternate_trials);
        ADD_RECALL(out.reach,result.reach);
        ADD_RECALL(out.base_first,result.base_first); ADD_RECALL(out.base_final,result.base_final);
        ADD_RECALL(out.anchor_items,result.anchor_items); ADD_RECALL(out.anchor_correct,result.anchor_correct);
        ADD_RECALL(out.base_anchor_first,result.base_anchor_first); ADD_RECALL(out.base_anchor_final,result.base_anchor_final);
        for (Word r=0;r<cfg.regimes;++r) {
            ADD_RECALL(out.acquired_in[r],result.acquired_in[r]);
            if (world.mixed()) ADD_RECALL(out.acquired_subset[r],result.acquired_subset[r]);
        }
        for (Word i=0;i<recall_stages;++i) {
            auto &d=out.stage[i];
            const auto &s=result.stage[i];
            ADD_RECALL(d.correct,s.correct); ADD_RECALL(d.returning,s.returning); ADD_RECALL(d.identified,s.identified);
            ADD_RECALL(d.identified_correct,s.identified_correct); ADD_RECALL(d.oracle_units,s.oracle_units);
            ADD_RECALL(d.exceptions,s.exceptions); ADD_RECALL(d.transfer,s.transfer);
            for (Word r=0;r<cfg.regimes;++r) ADD_RECALL(d.by_regime[r],s.by_regime[r]);
        }
#undef ADD_RECALL
    }
}
}
#define MEMORY(n,l) \
    ENTRY void revisit##l##_##n(const Genome *g,const SynapticLayer *s,const AssociationLayer *a,const Word *live,Word total,RevisitScore *out,Settings c,Revisit cfg,RevisitArm arm,Word first,Word count) { revisit<n,l,false>(g,s,a,live,total,out,nullptr,c,cfg,arm,first,count); } \
    ENTRY void rehearse##l##_##n(const Genome *g,const SynapticLayer *s,const AssociationLayer *a,Aptitude *out,Settings c,Revisit cfg,Word first,Word count) { revisit<n,l,true>(g,s,a,nullptr,0,nullptr,out,c,cfg,RevisitArm::intact,first,count); }
#define MEMORIES(n) MEMORY(n,0) MEMORY(n,1) MEMORY(n,2) MEMORY(n,3)
MEMORIES(2)
MEMORIES(4)
MEMORIES(8)
MEMORIES(16)
MEMORIES(32)
#undef MEMORIES
#undef MEMORY
