// Artificial selection is an optional founding treatment. Its descendants
// enter the ordinary spatial ecology with their lifetime neural state reset.
namespace {
AX_FN Word merit(const Aptitude &f, Word key, NurseryCourse course) {
    if (course==NurseryCourse::repertoire || course==NurseryCourse::integrated) {
        Word worst=f.score[0], sum=0;
        UNROLL
        for (Word v:f.score) { worst=lesser(worst,v); sum+=v; }
        Word primary=(key&3) ? worst : f.score[(key>>8)%12];
        return primary*512+sum;
    }
    if (course==NurseryCourse::recall) {
        Word low=lesser(f.score[6],f.score[7]);
        Word primary=(key&7) ? low : f.score[6+((key>>8)&1)];
        return primary*512+f.score[6]+f.score[7];
    }
    bool varied=course!=NurseryCourse::paired && course!=NurseryCourse::outcomes && course!=NurseryCourse::subsistence && course!=NurseryCourse::navigation && course!=NurseryCourse::migration && course!=NurseryCourse::junctions;
    Word low = lesser(lesser(f.score[0], f.score[1]), lesser(f.score[2], f.score[3]));
    Word sum = f.score[0] + f.score[1] + f.score[2] + f.score[3];
    if (varied) { low = lesser(low,lesser(f.score[4],f.score[5])); sum += f.score[4]+f.score[5]; }
    if (course==NurseryCourse::lifelong) { low=lesser(low,lesser(f.score[6],f.score[7])); sum+=f.score[6]+f.score[7]; }
    Word primary = (key & 7) ? low : f.score[course==NurseryCourse::lifelong ? (key>>8)&7 : varied ? (key >> 8) % 6 : (key >> 8) & 3];
    return primary * 512 + sum;
}
AX_FN Word breeder(const Aptitude *fitness,Word sites,Word key,NurseryCourse course,bool specialist) {
    Word first=course==NurseryCourse::recall ? 6 : 0;
    Word n=course==NurseryCourse::recall ? 2 : (course==NurseryCourse::paired || course==NurseryCourse::outcomes || course==NurseryCourse::subsistence || course==NurseryCourse::navigation || course==NurseryCourse::migration || course==NurseryCourse::junctions) ? 4 :
           course==NurseryCourse::varied ? 6 : course==NurseryCourse::lifelong ? 8 : 12;
    Word measure=first+(key>>8)%n;
    Word selected=mix(key)&(sites-1);
    Word best=specialist ? fitness[selected].score[measure] : merit(fitness[selected],key,course);
    UNROLL
    for (int j=1;j<8;++j) {
        Word rival=mix(key+389u*j)&(sites-1);
        Word rank=specialist ? fitness[rival].score[measure] : merit(fitness[rival],key,course);
        if (rank>best) { best=rank; selected=rival; }
    }
    return selected;
}
}
ENTRY void life_brood(const Genome *parents, Genome *offspring, const SynapticLayer *layers, SynapticLayer *young,
                      const Aptitude *fitness, Word sites, Word seed, Word generation, Word drift, Word mutate, const AssociationLayer *links, AssociationLayer *new_links, NurseryCourse course, Word level, Word programs,const OutcomeLayer *outcomes,OutcomeLayer *young_outcomes,Word repair,Word navigation) {
    Word id = index();
    if (id >= sites) return;
    Word key = trial_key(seed ^ 0x8fd7c4a1u, id, generation);
    Word winner = mix(key) & (sites-1);
    Word span=sites,region=0;
    if (drift==8 && (mix(key^0x37f94eadu)&31)) {
        span=lesser(sites,64);region=id&~(span-1);
    }
    if (drift!=1) winner=region+breeder(fitness+region,span,key,course,false);
    Word partner=winner, segment=0;
    if ((drift==2 || drift==4) && (id&7)) {
        partner=breeder(fitness,sites,mix(key^0xc56780a1u),course,true);
        segment=mix(key^0xb5b099d5u);
    }
    Genome embryo=parents[winner];
    if (segment && drift==2) {
        const auto &other=parents[partner];
        if (segment&1) {
            UNROLL
            for (int i=0;i<8;++i) embryo.op[i]=other.op[i];
            embryo.expression=(embryo.expression&0xffff0000u)|(other.expression&65535);
        }
        if (segment&2) {
            embryo.edges[0]=other.edges[0]; embryo.edges[1]=other.edges[1];
            embryo.development=other.development; embryo.receptors=other.receptors; embryo.motors=other.motors;
        }
        if (segment&4) embryo.expression=(embryo.expression&65535)|(other.expression&0xffff0000u);
        if (segment&8) {
            embryo.body=other.body; embryo.reproduction=other.reproduction;
            embryo.effectors[0]=other.effectors[0]; embryo.effectors[1]=other.effectors[1];
        }
    }
    Word edits = 0, structural = 0;
    bool change = mutate && (id & 7);
    bool core_change=change && drift<3;
    Word regions=mutable_regions(embryo.reproduction,key,3+Word(young_outcomes!=nullptr),repair);
    bool synaptic_change=change && (regions&4);
    inherit(embryo,offspring[id],key,core_change,edits,structural,regions);
    if (new_links) {
        Word probability=4+((embryo.reproduction>>8)&31), network=drift==2 && (segment&16) ? partner : winner;
        new_links[id]={inherit_links(links[network].genes,key,probability,core_change && (regions&4),edits),{}, {},
                       inherit_convergence(links[network].convergence,key,probability,core_change && (regions&4),edits)};
        if (level==3) {
            auto parent=context_tail(links,sites)[winner].genes;
            if (segment) {
                if (programs==2 && (segment&0x4000u)) parent=expression_tail(links,sites)[winner].genes.alternative;
                const auto &other=context_tail(links,sites)[partner].genes;
                parent.plastic=splice_program(parent.plastic,other.plastic,mix(segment+1));
                parent.evidence=splice_program(parent.evidence,other.evidence,mix(segment+2));
                parent.dynamics=splice_program(parent.dynamics,other.dynamics,mix(segment+3));
                parent.coordination=splice_program(parent.coordination,other.coordination,mix(segment+4));
                parent.receptors=splice_program(parent.receptors,other.receptors,mix(segment+5));
            }
            context_tail(new_links,sites)[id]={inherit_context(parent,embryo,links[network].genes,key,probability,synaptic_change && drift<5,edits),{},0,0,0};
            if (programs==2) {
                auto regulator=expression_tail(links,sites)[winner].genes;
                if (segment) {
                    auto &other=expression_tail(links,sites)[partner].genes;
                    if (segment&1024) regulator.alternative=segment&2048 ? other.alternative : context_tail(links,sites)[partner].genes;
                    regulator.gate=splice_program(regulator.gate,other.gate,mix(segment+6));
                    regulator.sensors=splice_program(regulator.sensors,other.sensors,mix(segment+7));
                }
                expression_tail(new_links,sites)[id]={inherit_expression(regulator,parent,embryo,links[network].genes,key,probability,synaptic_change && drift<5,edits),0};
            }
        }
    }
    if (young_outcomes) {
        auto parent=outcomes[winner].genes;
        if (segment) {
            auto other=outcomes[partner].genes;
            parent.plastic=splice_program(parent.plastic,other.plastic,mix(segment+13));
            parent.gates=splice_program(parent.gates,other.gates,mix(segment+17));
            if (segment&256) parent.wiring=other.wiring;
            if (segment&512) parent.valuation=other.valuation;
        }
        if (navigation) {
            if (navigation>=4) {
                const auto *histories=trace_tail(outcomes,sites);
                TraceGenes program=histories[segment&512 ? partner : winner].genes;
                if (drift==8 && change && (regions&8)) {
                    Word second=region+breeder(fitness+region,span,mix(key^0xea6d53b7u),course,true);
                    const auto &other=histories[second].genes;
                    program.plastic=splice_program(program.plastic,other.plastic,mix(key+7));
                    program.charge=splice_program(program.charge,other.charge,mix(key+11));
                    program.attention=splice_program(program.attention,other.attention,mix(key+17));
                    Word draw=mix(key^0x1273de5bu),chance=4+((embryo.reproduction>>8)&31);
                    if ((draw&255)<chance) {
                        Word source=((draw>>8)&7)*4,dest=((draw>>11)&7)*4;
                        program.charge=(program.charge&~(15u<<dest))|(((program.charge>>source)&15)<<dest);
                    }
                }
                trace_tail(young_outcomes,sites)[id]={inherit_trace(program,embryo,key,change && (drift<3 || drift==7 || drift==8) && (regions&8),edits),{},{},0,0,0};
            }
            const auto &p=map_tail(outcomes,sites)[segment&512 ? partner : winner];
            map_tail(young_outcomes,sites)[id]={inherit_map(p.genes,embryo,parent,key,change && (drift<3 || drift==6) && (regions&8),edits),{},{},0};
        }
        young_outcomes[id]={inherit_outcomes(parent,embryo,key,change && drift<6 && (regions&8),edits),{},0,0};
    }
    if (young) young[id] = {inherit_synapse(layers[drift==2 && (segment&4) ? partner : winner].genes,embryo,key,core_change && (regions&4),edits), {}};
}

ENTRY void temporal_founders(OutcomeLayer *organs,Word sites,Word seed,Word origin) {
    Word id=index();if (id>=sites) return;
    Word key=mix(seed^id^0xba4d3291u);
    auto p=trace_founder(key);
    if (origin==2) p.charge=mix(key^0x381ab65du);
    trace_tail(organs,sites)[id].genes=p;
}
