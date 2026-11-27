#include "axion/census.hpp"
// The breeding arena uses the same inherited instruction, chemistry and
// plasticity mutations as births in the shared habitat.
#include "life.cu"
#include "axion/dialogue.hpp"
#include "axion/sequence.hpp"
#include "axion/composition.hpp"
#include "axion/procedure.hpp"
#include "axion/grounding.hpp"
#include "axion/habitat_view.hpp"
ENTRY void voice_find_resident(const Site *grid,Word side,Word x,Word y,ResidentView wanted,Word nearest,Word *best) {
    Word pos=index(),area=side*side,key=~0u;
    if (pos<area) {
        const auto &b=grid[pos].body;
        if (b.energy && (nearest || (b.slot==wanted.slot && b.founder==wanted.founder && b.born==wanted.born && b.generation==wanted.generation))) {
            Word dx=(pos%side+side-x)&(side-1),dy=(pos/side+side-y)&(side-1);
            dx=dx<side-dx ? dx : side-dx;dy=dy<side-dy ? dy : side-dy;
            key=(dx+dy)*area+pos;
        }
    }
    Word minimum;asm("redux.sync.min.u32 %0, %1, 0xffffffff;":"=r"(minimum):"r"(key));
    if (!(pos&31) && minimum!=~0u) asm volatile("red.global.min.u32 [%0], %1;"::"l"(best),"r"(minimum):"memory");
}
namespace {
AX_FN void mutate_voice(Propagule &p,Word key,VoiceBody form) {
    const Genome parent=p.genome;
    Word edits=0,structural=0,probability=4+((parent.reproduction>>8)&31);
    inherit(parent,p.genome,key,true,edits,structural);
    if (form.bits==2) p.dendrites=inherit_synapse(p.dendrites,parent,key,true,edits);
    if (form.relations) {
        p.paths.genes=inherit_links(p.paths.genes,key,probability,true,edits);
        p.paths.convergence=inherit_convergence(p.paths.convergence,key,probability,true,edits);
    }
    if (form.relations==3) p.context=inherit_context(p.context,parent,p.paths.genes,key,probability,true,edits);
    if (form.alternative) p.expression=inherit_expression(p.expression,p.context,parent,p.paths.genes,key,probability,true,edits);
    if (form.balanced) p.outcomes=inherit_outcomes(p.outcomes,parent,key,true,edits);
    if (form.navigation) p.map=inherit_map(p.map,parent,p.outcomes,key,true,edits);
    if (form.navigation>=4) p.trace=inherit_trace(p.trace,parent,key,true,edits);
}
}
ENTRY void voice_hatch(const VoiceGermline *pool,unsigned char *bodies,unsigned char *paths,unsigned char *organs,
                      VoiceBody form,Word replicates,Word seed,AuditoryLayer *ears,Recollection *recall,AttentiveEar *caller,Iris *iris,Bindings *bindings,Word scattered,Conjunctions *joint,Reports *reports,Concepts *concepts,Agency *agency,SequenceMind *sequence,composition::Mind *composition_body,procedure::Mind *practice,grounding::Mind *ground) {
    Word i=index();if (i>=form.shape.copies) return;
    Word families=form.shape.copies/replicates;
    const auto &germ=pool[scattered ? i%families : i/replicates];
    const auto &p=germ.body;
    if (sequence) sequence[i].genes=germ.sequence;
    if (composition_body) composition_body[i].genes=germ.composition;
    if (practice) practice[i].genes=germ.procedure;
    if (ground) {ground[i].genes=germ.grounding;ground[i].version=1;}
    if (recall) recall[i].genes=germ.recall;
    if (ears) ears[i].rule=germ.auditory_rule;
    if (caller) caller[i].rule=germ.attention_rule;
    if (iris) iris[i].rule=germ.iris_rule;
    if (bindings) bindings[i].genes=germ.binding;
    if (joint) joint[i].gates=germ.gates;
    if (reports) reports[i].genes=germ.report;
    if (concepts) concepts[i].genes=germ.concepts;
    if (agency) {agency[i].model.genes=germ.agency;agency[i].intention.object=32;}
    auto *a=reinterpret_cast<Individual *>(bodies+Count(i)*form.shape.body_bytes);
    a->genes=p.genome;a->tiles=form.tiles;a->bits=form.bits;a->seed=mix(seed^(scattered ? i/families : i%replicates));
    a->has_dendrites=form.dendrites;a->has_relations=form.relations;a->alternate_program=form.alternative;
    a->delayed_path=form.delayed;a->outcome_lag=form.outcome_lag;a->layer.genes=p.dendrites;
    if (form.relations) {
        auto *links=reinterpret_cast<AssociationLayer *>(paths+Count(i)*form.shape.path_bytes);
        links->genes=p.paths.genes;links->convergence=p.paths.convergence;
        if (form.relations==3) context_tail(links,1)->genes=p.context;
        if (form.alternative) expression_tail(links,1)->genes=p.expression;
    }
    if (form.balanced) {
        auto *food=reinterpret_cast<OutcomeLayer *>(organs+Count(i)*form.shape.organ_bytes);
        food->genes=p.outcomes;
        if (form.navigation) map_tail(food,1)->genes=p.map;
        if (form.navigation>=4) trace_tail(food,1)->genes=p.trace;
    }
}
ENTRY void voice_merit(const DialogueScore *scores,Word *fitness,Word pairs,Word replicates,Word second) {
    Word candidate=index();if (candidate>=pairs/replicates) return;
    Word total=0;
    for (Word j=0;j<replicates;++j) total+=scores[candidate*replicates+j].correct;
    if (!second) fitness[candidate]=total;
    else {
        Word first=fitness[candidate];
        fitness[candidate]=(first<total ? first : total)*4+first+total;
    }
}
namespace {
AX_FN void mutate_voice_bundle(VoiceGermline &p,VoiceBody form,Word draw) {
    Word chance=4+((p.body.genome.reproduction>>8)&31);
    Word change=mix(draw^0x982b5c61u);
    if (form.attends && (change&255)<chance) p.attention_rule^=1u<<((change>>8)&31);
    if (form.orients && ((change>>16)&255)<chance) p.iris_rule^=1u<<((change>>24)&31);
    if (form.binds) {
        Word *loci[]{&p.binding.plastic,&p.binding.competition,&p.binding.exploration};
        for (Word k=0;k<3;++k) {
            Word edit=mix(change^(k+1)*0x267d11b9u);
            if ((edit&255)<chance) *loci[k]^=1u<<((edit>>8)&31);
        }
    }
    if (form.auditory && (draw&255)<4+((p.body.genome.reproduction>>8)&31)) p.auditory_rule^=1u<<((draw>>13)&31);
    if (form.conjoins) {
        Word *loci[]{&p.gates.low,&p.gates.high,&p.gates.expression};
        for (Word k=0;k<3;++k) {
            Word edit=mix(change^(k+1)*0xe6723bb5u);
            if ((edit&255)<chance) *loci[k]^=1u<<((edit>>8)&31);
        }
    }
    if (form.recollects) {
        for (Word k=0;k<3;++k) {
            Word edit=mix(draw^((k+1)*0x95e6b21du));
            if ((edit&255)<chance) {
                Word bit=1u<<((edit>>9)&31);
                if (!k) p.recall.low^=bit;else if (k==1) p.recall.high^=bit;else p.recall.expression^=bit;
            }
        }
    }
    if (form.reports) {
        Word *loci[]{&p.report.low,&p.report.high,&p.report.expression};
        for (Word k=0;k<3;++k) {
            Word edit=mix(change^(k+1)*0xc9d3bf51u);
            if ((edit&255)<chance) *loci[k]^=1u<<((edit>>8)&31);
        }
    }
    if (form.grounded) {
        Word *loci[]{&p.grounding.low,&p.grounding.high,&p.grounding.expression};
        for (Word k=0;k<3;++k) {
            Word edit=mix(change^(k+1)*0x713be8d5u);
            if ((edit&255)<chance) *loci[k]^=1u<<((edit>>8)&31);
        }
    }
    if (form.procedural) {
        Word *loci[]{&p.procedure.low,&p.procedure.high,&p.procedure.expression};
        for (Word k=0;k<3;++k) {
            Word edit=mix(change^(k+1)*0x69ad138bu);
            if ((edit&255)<chance) *loci[k]^=1u<<((edit>>8)&31);
        }
    }
    if (form.composes) {
        Word *loci[]{&p.composition.low,&p.composition.high,&p.composition.expression};
        for (Word k=0;k<3;++k) {
            Word edit=mix(change^(k+1)*0x3da776b1u);
            if ((edit&255)<chance) *loci[k]^=1u<<((edit>>8)&31);
        }
    }
    if (form.sequential) {
        Word *loci[]{&p.sequence.low,&p.sequence.high,&p.sequence.expression};
        for (Word k=0;k<3;++k) {
            Word edit=mix(change^(k+1)*0xe63a99d7u);
            if ((edit&255)<chance) *loci[k]^=1u<<((edit>>8)&31);
        }
    }
    if (form.causal && (mix(draw+2591)&3)==0) {
        Word k=mix(draw+3271),*loci[]{&p.agency.low,&p.agency.high,&p.agency.expression};*loci[k%3]^=1u<<((k>>8)&31);
    }
    if (form.conceptual) {
        Word *loci[]{&p.concepts.low,&p.concepts.high,&p.concepts.expression};
        for (Word k=0;k<3;++k) {
            Word edit=mix(change^(k+1)*0x875ca00du);
            if ((edit&255)<chance) *loci[k]^=1u<<((edit>>8)&31);
        }
    }
    mutate_voice(p.body,draw,form);
}
}
ENTRY void voice_select(const VoiceGermline *parents,VoiceGermline *offspring,const Word *fitness,Word count,Word seed,
                        Word drift,VoiceBody speaker,VoiceBody listener) {
    Word i=index();if (i>=count) return;
    Word key=mix(seed^i*0xd1b54a35u),winner=mix(key)%count;
    if (!drift) for (Word draw=1;draw<7;++draw) {
        Word rival=mix(key+draw*0x64c1f8b7u)%count;
        if (fitness[rival]>fitness[winner]) winner=rival;
    }
    for (Word role=0;role<2;++role) {
        auto p=parents[winner+role*count];
        auto form=role ? listener : speaker;
        Word draw=mix(key^role*0x7a5b20c3u);
        mutate_voice_bundle(p,form,draw);
        offspring[i+role*count]=p;
    }
}
ENTRY void voice_survey(const Word *fitness,Word count,Count *summary) {
    Word lane=index(),best=0,total=0;
    Count sum=0;
    for (Word i=lane;i<count;i+=128) {
        Word score=fitness[i];sum+=score;
        if (score>total || (score==total && i<best)) {total=score;best=i;}
    }
    __attribute__((shared)) static Count sums[128];
    __attribute__((shared)) static Word values[128],which[128];
    sums[lane]=sum;values[lane]=total;which[lane]=best;
    for (Word offset=64;offset;offset>>=1) {
        asm volatile("bar.sync 0;":::"memory");
        if (lane<offset) {
            sums[lane]+=sums[lane+offset];
            if (values[lane+offset]>values[lane] || (values[lane+offset]==values[lane] && which[lane+offset]<which[lane])) {
                values[lane]=values[lane+offset];which[lane]=which[lane+offset];
            }
        }
    }
    if (!lane) {summary[0]=sums[0];summary[1]=values[0];summary[2]=which[0];}
}
#include "axion/habitat_view.hpp"
ENTRY void voice_field_view(const Site *field,Settings c,Word tick,Word x,Word y,Word radius,HabitatSignals *out) {
    Word lane=index(),width=2*radius+1,sums[8]{};
    for (Word k=lane;k<width*width;k+=128) {
        int dx=int(k%width)-int(radius),dy=int(k/width)-int(radius);
        int ax=dx<0 ? -dx : dx,ay=dy<0 ? -dy : dy;
        if (!ax && !ay) continue;
        Word bearing=ax>ay ? (dx>0 ? 1u : 3u) : (dy>0 ? 2u : 0u);
        Word weight=1+radius-Word(ax>ay ? ax : ay),pos=cell_at(x+dx,y+dy,c.side);
        sums[bearing]+=field[pos].food*weight;
        if (c.springs && aquifer(pos,local_climate(pos,tick,c,nullptr).low,c)) sums[4+bearing]+=c.springs*weight;
    }
    __attribute__((shared)) static Word tiles[4][8];
    for (Word k=0;k<8;++k) {
        Word sum=group_sum<32>(sums[k],lane&31,lane&~31u,~0u);
        if (!(lane&31)) tiles[lane>>5][k]=sum;
    }
    asm volatile("bar.sync 0;":::"memory");
    if (!lane) {
        Word strongest[2]{},bearing[2]{4,4};
        for (Word k=0;k<8;++k) {
            Word current=tiles[0][k]+tiles[1][k]+tiles[2][k]+tiles[3][k];
            (k<4 ? out->food : out->water)[k&3]=current;
            if (current>strongest[k/4]) {strongest[k/4]=current;bearing[k/4]=k&3;}
        }
        out->food_bearing=bearing[0];out->water_bearing=bearing[1];
    }
}

#include "commons.cuh"
