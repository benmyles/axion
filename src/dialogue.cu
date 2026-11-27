#include "axion/dialogue.hpp"
#include "recollection.cuh"
#include "attention.cuh"
#include "imagery.cuh"
#include "conjunction.cuh"
#include "report.cuh"
#include "concept.cuh"
using namespace axion;
using namespace axion::life;
#define KERNEL extern "C" __attribute__((global))
namespace {
AX_FN Word index() {
    Word block,thread,width;
    asm("mov.u32 %0, %%ctaid.x;" : "=r"(block));
    asm("mov.u32 %0, %%tid.x;" : "=r"(thread));
    asm("mov.u32 %0, %%ntid.x;" : "=r"(width));
    return block*width+thread;
}
AX_FN Word sense(Word key,Word tag) {
    return sensory_address(key,tag);
}
// The habitat's delayed peripheral event fits in its low 16 bits. The
// detached animal can retain a copy of a sound it actually emitted above it.
AX_FN Word last_call(const Individual *a) { return (a->contacts.previous>>16)&7; }
AX_FN void retain_call(Individual *a,Word call) {
    a->contacts.previous=(a->contacts.previous&~(7u<<16))|((call+1)<<16);
}
AX_FN Word context_tag(Word slot,Word saved,Wire cut) {
    Word call=saved ? (saved-1)&3 : 0;
    if (cut==Wire::muted_self) call=0;
    if (cut==Wire::shifted_self) call=(call+1)&3;
    return 16+4*slot+call;
}
AX_FN Word query_tag(const Exchange &w,DialogueConfig cfg) {
    if (cfg.discourse) return context_tag((cfg.nouns>>(4*w.form))&3,w.self,cfg.wire);
    return 16+(cfg.compass ? (cfg.nouns>>(4*w.topic))&15 : w.topic);
}
AX_FN Word word_receptors(Word key,Word tag,Word fingerprint,Word self,bool distributed) {
    Word raw=sense(key,tag);
    if (distributed) {
        Word f=mix(fingerprint),context=self ? (self-1)&3 : 0;
        raw|=sense(key,((f>>12)+context)&3);
        raw|=sense(key,4+(((f>>18)+context)&3));
        raw|=sense(key,12+(((f>>24)+context)&3));
    }
    return raw;
}
AX_FN void reference_scene(Exchange &w,Word key) {
    // A named question establishes a referent. The next question repeats it
    // or requests the other one. The requested referent is private to scoring.
    Word pair=w.round/2,draw=mix(key^((pair/64)*0x6394f213u));
    Word item=((pair&63)*((draw>>8)|1)+draw)&63;item^=item>>3;
    w.shape=item&3;w.colour=(item>>2)&3;
    Word anchor=(item>>4)&1,other=item>>5;
    w.form=(w.round&1) ? 2+other : anchor;
    w.topic=anchor^Word((w.round&1) && other);
}
AX_FN void separate_scenes(Exchange &w,DialogueConfig cfg) {
    if (cfg.withheld) {
        Word parity=(w.shape^w.colour)&1;
        if (parity!=Word(!cfg.learning)) w.shape^=1;
    }
}
AX_FN Word phoneme(Word ports) { return ports&8 ? ports&3 : 4; }
AX_FN Word room_key(Word pair,Word seed) {return mix(seed^pair*0x9e3779b9u);}
AX_FN Word class_key(Word pair,DialogueConfig cfg) { return room_key(cfg.worlds ? pair%cfg.worlds : pair/cfg.community,cfg.seed); }
AX_FN Word circuit(Word key,Word value) {
    // An invertible two-bit mapping with all 24 permutations available.
    Word symbols=0xe4;
    for (Word n=4;n>1;--n) {
        Word i=key%n;key/=n;
        Word a=2*i,b=2*(n-1),swap=((symbols>>a)^(symbols>>b))&3;
        symbols^=(swap<<a)|(swap<<b);
    }
    return (symbols>>(2*value))&3;
}
AX_FN Individual *individual(unsigned char *data,Word i,Word stride) {
    return reinterpret_cast<Individual *>(data+Count(i)*stride);
}
AX_FN Word remap(Word raw,Count address,Word pair,Word role) {
    if (!address) return raw;
    const auto &w=reinterpret_cast<const OpticalMap *>(address)[pair];
    Word result=0;
    while (raw) {
        Word tag=Word(__builtin_ctz(raw)),projected=0;raw&=raw-1;
        for (Word bit=0;bit<5;++bit) projected|=((w.planes[role][bit]>>tag)&1)<<bit;
        result|=1u<<projected;
    }
    return result;
}
AX_FN Encounter observe(const Individual *brain,Word input,bool learning,Count optics,Word pair,Word role) {
    input=remap(input,optics,pair,role);
    Word signal=(brain->due>>(2*(brain->ticks&7)))&3;
    return {input,0x01070000u,0,0,signal,learning ? EventKind::step : EventKind::probe};
}
AX_FN Word recognition(const AuditoryLayer &ear,Word key) {
    for (Word j=0;j<16;++j) if (ear.forms[j]==key) {
        Word q=(ear.efficacy[j/8]>>(4*(j&7)))&15;
        if (!q) continue;
        Word paths=ear.projections[j/2]>>(16*(j&1)),best=paths&15,projection=0;
        for (Word k=1;k<4;++k) {
            Word drive=(paths>>(4*k))&15;
            if (drive>best) { best=drive;projection=k; }
        }
        return q|(projection<<4);
    }
    return 0;
}
AX_FN void learn_sound(AuditoryLayer &ear,Word key,bool gain,const Genome &g,Word projection=0) {
    Word first=mix(key^g.receptors)&15,site=first,lowest=16;bool found=false;
    for (Word k=0;k<16;++k) {
        Word j=(k+first)&15,level=(ear.efficacy[j/8]>>(4*(j&7)))&15;
        if (level && ear.forms[j]==key) { site=j;lowest=level;found=true;break; }
        if (level<lowest) { site=j;lowest=level; }
    }
    Word threshold=2+((g.receptors>>15)&3);
    if (!found && lowest>=threshold) return;
    Word &branches=ear.projections[site/2],row=16*(site&1),shift=row+4*projection;
    Word old_paths=found ? branches>>row : 0;
    Word strength=auditory_charge((old_paths>>(4*projection))&15,gain,threshold,ear.rule);
    if (!found && !strength) return;
    if (!found) branches&=~(0xffffu<<row);
    branches=(branches&~(15u<<shift))|(strength<<shift);
    // The strongest projection can admit the sound even while other
    // candidate connections are being sampled and losing charge.
    Word paths=branches>>row,level=0;
    for (Word k=0;k<4;++k) {Word q=(paths>>(4*k))&15;level=q>level ? q : level;}
    ear.forms[site]=key;Word &bank=ear.efficacy[site/8];shift=4*(site&7);
    bank=(bank&~(15u<<shift))|(level<<shift);
}
AX_FN void reward(Individual *brain,bool gain) {
    Word shift=2*((brain->ticks-1+brain->outcome_lag)&7);
    brain->due=(brain->due&~(3u<<shift))|((gain ? 1u : 2u)<<shift);
}
}
KERNEL void voice_replicate(const unsigned char *source,unsigned char *dest,Word bytes,Word count) {
    Word i=index();
    if (i>=bytes/4 && i<Count(bytes/4)*count) reinterpret_cast<Word *>(dest)[i]=reinterpret_cast<const Word *>(source)[i%(bytes/4)];
}
KERNEL void voice_seed(unsigned char *data,Word stride,Word count,Word salt) {
    Word i=index();if (i>=count) return;
    // Pair zero preserves the public individual's exact seed so its dialect
    // can be replayed directly from a saved checkpoint.
    if (i) { auto *a=individual(data,i,stride);a->seed=mix(a->seed^i^salt); }
}
KERNEL void voice_scene(Exchange *world,Encounter *events,unsigned char *brains,Word stride,DialogueConfig cfg) {
    Word i=index();if (i>=cfg.pairs) return;
    auto &w=world[i];Word key=class_key(i,cfg),block=mix(key^((w.round/32)*0x63d81b79u));
    Word pattern=((w.round&31)*((block>>8)|1)+block)&31;
    pattern^=pattern>>3;
    w.shape=pattern&3;w.colour=(pattern>>2)&3;w.topic=cfg.facets==2 ? pattern>>4 : 0;w.question=4;w.looked=2;w.form=w.topic;
    if (cfg.discourse) {if (!w.round) w.self=last_call(individual(brains,i,stride));reference_scene(w,key);}
    separate_scenes(w,cfg);
    Word received=cfg.facets==2 ? word_receptors(key^0x18a3942bu,query_tag(w,cfg),cfg.forms[w.form],cfg.discourse ? 1+(context_tag(0,w.self,cfg.wire)&3) : 0,cfg.distributed) : sense(key,cfg.wire==Wire::hidden ? 0 : w.shape);
    events[i]=observe(individual(brains,i,stride),received,cfg.learning && (cfg.plasticity&(cfg.facets==2 ? 2u : 1u)),cfg.optics,i,Word(cfg.facets==2));
}
KERNEL void voice_question(Exchange *world,const Response *asks,Encounter *events,unsigned char *brains,Word stride,DialogueConfig cfg,unsigned char *askers,Word asker_stride) {
    Word i=index();if (i>=cfg.pairs) return;
    auto &w=world[i];Word key=class_key(i,cfg);
    w.question=phoneme(asks[i].ports);
    w.attention=0;
    if (cfg.caller && cfg.iris) {
        Word pitch=reinterpret_cast<const AttentiveEar *>(cfg.caller)[i].previous;
        const auto &muscle=reinterpret_cast<const Iris *>(cfg.iris)[i];
        Word heard=pitch^Word(cfg.pitch_cut==2);
        Word gate=cfg.pitch_cut==1 ? 2u : attention::response(muscle.contacts,heard);
        w.attention=4|(pitch<<1)|Word(gate==1);
    }
    if (cfg.discourse) {
        w.self=w.question+1;
        if (cfg.learning && (cfg.plasticity&2)) retain_call(individual(askers,i,asker_stride),w.question);
    }
    Word heard=w.question;
    if (cfg.wire==Wire::no_question) heard=4;
    if (cfg.wire==Wire::shift_question && heard<4) heard=(heard+1)&3;
    Word raw=sense(key,cfg.wire==Wire::hidden ? 0 : w.shape)|sense(key,4+(cfg.wire==Wire::hidden ? 0 : w.colour))|sense(key,20+heard);
    if (cfg.active_vision) raw=sense(key,20+heard);
    events[i]=observe(individual(brains,i,stride),raw,cfg.learning && (cfg.plasticity&1),cfg.optics,i,0);
}
KERNEL void voice_answer(Exchange *world,const Response *speaker,Encounter *events,unsigned char *brains,Word stride,DialogueConfig cfg) {
    Word i=index();if (i>=cfg.pairs) return;
    auto &w=world[i];Word key=class_key(i,cfg)^0x18a3942bu;
    w.voice=phoneme(speaker[i].ports);
    Word heard=w.voice;
    if (cfg.wire==Wire::stranger) {
        Word start=(i/cfg.community)*cfg.community,partner=start+(i-start+1)%cfg.community;
        heard=phoneme(speaker[partner].ports);
    }
    if (cfg.wire==Wire::silent) heard=4;
    if (cfg.wire==Wire::shifted && heard<4) heard=(heard+1)&3;
    Word raw=sense(key,8+heard);
    if (cfg.facets==2 && !cfg.active_vision) raw|=sense(key,16+w.topic);
    events[i]=observe(individual(brains,i,stride),raw,cfg.learning && (cfg.plasticity&2),cfg.optics,i,1);
}
KERNEL void voice_result(Exchange *world,const Response *listener,DialogueScore *scores,unsigned char *sender,Word a_bytes,
                         unsigned char *receiver,Word b_bytes,DialogueConfig cfg,AuditoryLayer *ears) {
    Word i=index();if (i>=cfg.pairs) return;
    auto &w=world[i];auto &s=scores[i];
    Word fact=w.topic ? w.colour : w.shape;
    Word target=(circuit(mix(class_key(i,cfg)^0x46307bfdu),fact)+cfg.reversal)&3;
    Word choice=phoneme(listener[i].ports);
    bool gain=choice==target;
    if (!cfg.worlds) {
        s.relative+=cfg.discourse && w.form>=2;s.relative_correct+=cfg.discourse && w.form>=2 && gain;
        ++s.attempts;s.voiced+=w.voice!=4;s.moved+=choice!=4;
        s.relevant_gazes+=w.looked==w.topic;
        ++s.calls[fact][w.voice];++s.requests[w.topic][w.question];
    }
    if (!cfg.worlds || !cfg.learning) s.correct+=gain;
    if (cfg.learning && cfg.outcomes!=1) {
        bool fed=gain;
        if (cfg.outcomes==2) {
            // A one-to-one rotation preserves the population's number of food
            // pulses on this tick while disconnecting each pair's own outcome.
            Word stride=cfg.community<cfg.pairs ? cfg.community : 1;
            Word peer=i+stride;if (peer>=cfg.pairs) peer-=cfg.pairs;
            auto &other=world[peer];
            Word value=other.topic ? other.colour : other.shape;
            Word goal=(circuit(mix(class_key(peer,cfg)^0x46307bfdu),value)+cfg.reversal)&3;
            fed=phoneme(listener[peer].ports)==goal;
        }
        if (ears && cfg.facets==2 && (cfg.plasticity&2)) learn_sound(ears[i],cfg.forms[w.form],fed,individual(receiver,i,b_bytes)->genes);
        if (cfg.plasticity&1) reward(individual(sender,i,a_bytes),fed);
        if (cfg.plasticity&2) reward(individual(receiver,i,b_bytes),fed);
    }
    ++w.round;
}
KERNEL void voice_human(Encounter *events,unsigned char *brains,Word stride,Word pair,Word local,Word seed,Word tag,Word feedback,Word discourse,Word fingerprint,Word distributed,Word margin,const AuditoryLayer *ears,Word *gate,Word explore,Count optics,AttentiveEar *caller,ImaginedWord projection) {
    if (index()) return;
    auto *a=individual(brains,pair,stride);
    if (caller) attention::hear(caller[pair],fingerprint);
    if (projection.reporter) report::hear_prompt(*reinterpret_cast<Reports *>(projection.reporter),fingerprint,projection.first);
    if (projection.audience) report::hear_prompt(*reinterpret_cast<Reports *>(projection.audience),fingerprint,projection.first);
    if (projection.paths || projection.roles) {
        auto genes=reinterpret_cast<const Recollection *>(projection.visual)->genes;
        Word unary=0x8421u,j=6;
        if (projection.image) {
            const auto &image=*reinterpret_cast<const Imagery *>(projection.image);
            j=imagery::field(image,fingerprint,false);
            if (j<6) unary=imagery::readout(genes,image.weights[j]);
        }
        if (projection.paths) {
            auto &wave=*reinterpret_cast<Projection *>(projection.paths);
            if (projection.first) wave=Projection{};
            if (j<6) {wave.paths=imagery::compose(wave.paths,unary);++wave.actions;}
        }
        if (projection.roles) {
            auto &wave=*reinterpret_cast<RoleProjection *>(projection.roles);
            if (projection.first) wave=RoleProjection{};
            auto &bindings=*reinterpret_cast<const Bindings *>(projection.bindings);
            if (projection.scenes) {
                auto &scenes=*reinterpret_cast<SceneWave *>(projection.scenes);
                if (projection.first) scenes.active=0;
                conjunction::hear(scenes,wave,bindings,*reinterpret_cast<const Conjunctions *>(projection.joint),genes,fingerprint,unary);
            }
            binding::hear(wave,bindings,genes,fingerprint,unary);
        }
    }
    Word recognised=ears ? recognition(ears[pair],fingerprint) : 0;
    Word branch=explore<4 ? explore : recognised>>4;
    if (discourse) tag=context_tag((tag+branch)&3,last_call(a),Wire::open);
    events[0]=observe(a,word_receptors(room_key(local,seed)^0x18a3942bu,tag,fingerprint,discourse ? last_call(a) : 0,distributed),true,optics,pair,1);
    if (margin) events[0].mask=vocal_margin_report;
    if (gate) *gate=ears ? Word((recognised&15)>=2+((a->genes.receptors>>15)&3)) : 1;
    if (feedback) events[0].signal=feedback;
}

KERNEL void voice_demonstration(Exchange *world,Encounter *events,unsigned char *brains,Word stride,DialogueConfig cfg) {
    Word i=index();if (i>=cfg.pairs) return;
    auto &w=world[i];Word key=class_key(i,cfg),block=mix(key^((w.round/32)*0x63d81b79u));
    Word pattern=((w.round&31)*((block>>8)|1)+block)&31;pattern^=pattern>>3;
    w.shape=pattern&3;w.colour=(pattern>>2)&3;w.topic=cfg.facets==2 ? pattern>>4 : 0;
    Word raw=sense(key,w.shape);
    if (cfg.facets==2 && !cfg.active_vision) raw|=sense(key,4+w.colour)|sense(key,20+circuit(mix(key^0x9c346b27u),w.topic));
    events[i]=observe(individual(brains,i,stride),raw,true,cfg.optics,i,0);
}
KERNEL void voice_handle(Exchange *world,const Response *actor,DialogueScore *scores,unsigned char *brains,Word stride,DialogueConfig cfg) {
    Word i=index();if (i>=cfg.pairs) return;
    auto &w=world[i];auto &s=scores[i];Word key=class_key(i,cfg);
    Word fact=w.topic && !cfg.active_vision ? w.colour : w.shape,muscle=phoneme(actor[i].ports);
    bool fed=muscle==(cfg.compass ? fact : circuit(mix(key^0xd76233f1u),fact));
    reward(individual(brains,i,stride),fed);
    ++s.attempts;s.correct+=fed;s.voiced+=muscle!=4;s.moved+=muscle!=4;++s.calls[fact][muscle];++w.round;
}

namespace {
AX_FN Word inspect_view(const Exchange &w,Word side,DialogueConfig cfg,Word i) {
    Word sample=cfg.wire==Wire::hidden ? 0u : side ? w.colour : w.shape;
    Word external=sample<4 && !(w.attention&1) && !(w.shutters&(1u<<side)) ? 1u<<sample : 0;
    Word retinal=external;
    if (cfg.recall) {
        auto &layer=reinterpret_cast<Recollection *>(cfg.recall)[i];
        auto cut=static_cast<RecallCut>(cfg.recall_cut);
        if (!external && cut!=RecallCut::erased)
            retinal=engram::reinstate(layer,cut==RecallCut::crossed ? side^1 : side);
        if (cfg.learning && (cfg.plasticity&1) && cut!=RecallCut::fixed)
            engram::see(layer,side,external,external && (!(w.dim&(1u<<side))!=(cut==RecallCut::source)));
    }
    return retinal;
}
}
KERNEL void voice_glance(Exchange *world,const Response *speaker,Encounter *events,unsigned char *brains,Word stride,DialogueConfig cfg) {
    Word i=index();if (i>=cfg.pairs) return;
    auto &w=world[i];Word key=class_key(i,cfg);
    Word goal=(speaker[i].ports^((key>>13)&1))&1;
    Word side=cfg.wire==Wire::blind_gaze ? 0u : goal,retinal=0;w.looked=side;
    const auto *roles=reinterpret_cast<const RoleProjection *>(cfg.roles);
    if (roles && roles[i].events) {
        const auto *scenes=reinterpret_cast<const SceneWave *>(cfg.scenes);
        bool joint=scenes && scenes[i].active;
        Word apertures=joint ? conjunction::apertures(scenes[i],goal) : (roles[i].apertures>>(2*goal))&3;
        Word opened=cfg.wire==Wire::blind_gaze ? apertures&1 : apertures;
        Word left=(opened&1) ? inspect_view(w,0,cfg,i) : 0;
        Word right=(opened&2) ? inspect_view(w,1,cfg,i) : 0;
        auto g=reinterpret_cast<const Bindings *>(cfg.bindings)[i].genes;
        retinal=joint ? conjunction::answer(scenes[i],goal,left,right) : binding::answer(roles[i],goal,left,right,g);
        w.looked=opened==1 ? 0 : opened==2 ? 1 : 2;
        w.attention|=8|(1u<<8)|(goal<<9)|(opened<<10)|(left<<12)|(right<<16);
    } else {
        retinal=inspect_view(w,side,cfg,i);
        if (cfg.projection) {
            const auto &wave=reinterpret_cast<const Projection *>(cfg.projection)[i];
            if (wave.actions) {w.attention|=8|(retinal<<4);retinal=imagery::propagate(wave.paths,retinal);}
        }
    }
    w.retinal=retinal;
    Word raw=0;
    while (retinal) {
        Word bearing=Word(__builtin_ctz(retinal));retinal&=retinal-1;raw|=sense(key,bearing);
    }
    events[i]=observe(individual(brains,i,stride),raw,cfg.learning && (cfg.plasticity&1),cfg.optics,i,0);
}

// During joint play each organism can try a physical action independently.
// Correct actuator settings exist only in the apparatus that delivers food.
KERNEL void voice_play_scene(Exchange *world,Encounter *events,unsigned char *brains,Word stride,DialogueConfig cfg,Word part) {
    Word i=index();if (i>=cfg.pairs) return;
    auto &w=world[i];Word key=class_key(i,cfg);
    if (!part) {
        Word draw=mix(key^((w.round/32)*0x13c7ab95u));Word item=((w.round&31)*((draw>>8)|1)+draw)&31;
        item^=item>>3;w.shape=item&3;w.colour=(item>>2)&3;w.topic=item>>4;w.looked=2;w.form=w.topic;
        if (cfg.discourse) {if (!w.round) w.self=last_call(individual(brains,i,stride));reference_scene(w,key);}
        separate_scenes(w,cfg);
    }
    Word fact=w.topic ? w.colour : w.shape;
    Word ask=circuit(mix(key^0x9c346b27u),w.topic),call=cfg.compass ? fact : circuit(mix(key^0xd76233f1u),fact);
    Word raw=part==0 ? word_receptors(key^0x18a3942bu,query_tag(w,cfg),cfg.forms[w.form],cfg.discourse ? 1+(context_tag(0,w.self,cfg.wire)&3) : 0,cfg.distributed) : part==1 ? sense(key,20+ask)
             : part==2 ? sense(key,fact) : sense(key^0x18a3942bu,8+call);
    events[i]=observe(individual(brains,i,stride),raw,true,cfg.optics,i,Word(part==0 || part==3));
}
KERNEL void voice_play_food(Exchange *world,const Response *actor,DialogueScore *scores,unsigned char *brains,Word stride,DialogueConfig cfg,Word part,AuditoryLayer *ears) {
    Word i=index();if (i>=cfg.pairs) return;
    auto &w=world[i];auto &s=scores[i];Word key=class_key(i,cfg),pins=actor[i].ports;
    Word fact=w.topic ? w.colour : w.shape,call=phoneme(pins);
    Word target=part==0 ? circuit(mix(key^0x9c346b27u),w.topic) : part==2 ? (cfg.compass ? fact : circuit(mix(key^0xd76233f1u),fact))
                        : (circuit(mix(key^0x46307bfdu),fact)+cfg.reversal)&3;
    bool gain=part==1 ? (((pins^(key>>13))&1)==w.topic) : call==target;
    reward(individual(brains,i,stride),gain);
    ++s.attempts;s.correct+=gain;s.voiced+=call<4;s.moved+=call<4;
    if (ears && !part) learn_sound(ears[i],cfg.forms[w.form],gain,individual(brains,i,stride)->genes);
    if (cfg.discourse && !part) {w.self=call+1;retain_call(individual(brains,i,stride),call);}
    if (part==2) ++s.calls[fact][call];
    if (part==3) ++w.round;
}

KERNEL void voice_keep(Response *kept,const Response *response,Word first,const Word *gate,Word peak) {
    if (index()) return;
    if (first) *kept={0,0,0,0};
    if ((!gate || *gate) && (!kept->received || !peak || response->matched>kept->matched)) {
        *kept=*response;kept->received=1;
    }
}

KERNEL void voice_rehear(Individual *asker,const Response *request) {
    if (!index()) retain_call(asker,phoneme(request->ports));
}
KERNEL void voice_point(AuditoryLayer *ear,const Individual *asker,const Exchange *world,Word expected,Word form,Word projection) {
    if (!index()) learn_sound(*ear,form,world->looked==expected,asker->genes,projection);
}
KERNEL void voice_focus_lesson(AttentiveEar *caller,const Iris *iris,Word form,Word operation,Word key) {
    if (!index()) attention::teach_source(*caller,*iris,form,operation,key);
}
KERNEL void voice_motion_lesson(Imagery *image,const Recollection *memory,Word sound,Word from,Word to) {
    if (!index()) imagery::experience(*image,memory->genes,sound,from,to);
}
KERNEL void voice_pair_lesson(Bindings *b,const Recollection *m,Word form,Word before,Word after,Word credit,Conjunctions *joint) {
    if (!index()) conjunction::experience(*b,joint,m->genes,form,before,after,credit!=0);
}
KERNEL void voice_report_watch(Reports *witness,Reports *audience,Word left,Word right,Word shutters,Word shared) {
    if (index()) return;
    Word retinal=((left<4 && !(shutters&1)) ? 1u<<left : 0)|((right<4 && !(shutters&2)) ? 16u<<right : 0);
    if (witness) report::watch(witness->history,witness->genes,retinal);
    if (audience) report::watch(audience->history,audience->genes,shared ? retinal : 0);
}
KERNEL void voice_report_lesson(Reports *witness,Reports *reader,Word cue,Utterance *demonstration,Word shared) {
    if (index()) return;
    bool a=witness && report::learn(*witness,cue,*demonstration);
    bool b=reader && shared && report::learn(*reader,cue,*demonstration);
    demonstration->handled=Word(a)|(Word(b)<<1);
}
KERNEL void voice_report_say(Reports *speaker,Reports *addressee,Utterance *wire,Word cut) {
    if (index()) return;
    Utterance emitted{};
    if (speaker) {
        bool requested=speaker->request!=0;
        if (addressee) {
            requested|=addressee->request!=0;
            Word pulse=addressee->request ? addressee->frames[addressee->request-1].cue : 0;
            Word k=report::frame(*speaker,pulse,false);speaker->request=k<2 ? k+1 : 0;
        }
        emitted=report::speak(*speaker);
        emitted.handled|=requested;
    }
    *wire=emitted;
    if (addressee && emitted.handled) {
        if (cut==1) emitted.count=0;
        if (cut==2) for (Word i=0;i<emitted.count/2;++i) {
            Count tmp=emitted.sound[i];emitted.sound[i]=emitted.sound[emitted.count-i-1];emitted.sound[emitted.count-i-1]=tmp;
        }
        report::receive(*addressee,emitted);
    }
}
KERNEL void voice_report_converse(Reports *addressee,Utterance *wire,Word cue,Word retell,Word cut) {
    if (index() || !addressee) return;
    if (retell) {*wire=report::recount(*addressee,cue);return;}
    auto heard=*wire;
    if (cut==1) heard.count=0;
    if (cut==2) for (Word i=0;i<heard.count/2;++i) {
        Count tmp=heard.sound[i];heard.sound[i]=heard.sound[heard.count-1-i];heard.sound[heard.count-1-i]=tmp;
    }
    report::listen(*addressee,cue,heard);
}
KERNEL void concept_encounter(Concepts *a,Concepts *b,ConceptInput input,ConceptReply *output) {
    if (index()) return;
    ConceptReply result{};auto k=input.event;
    if (k==ConceptEvent::ask && a) {
        result.handled=meaning::ask(*a,input.form,input.a ? input.a : a->domain,input.b!=0);
        if (result.handled) {result.start=1u<<a->pending.source;result.sound=meaning::pronounce(*a,{result.start,0});}
    } else if (k==ConceptEvent::reply && a) result.handled=meaning::reply(*a,input.a);
    else if (k==ConceptEvent::cancel && a) {result.handled=a->pending.form!=0;a->pending={};}
    else if (k==ConceptEvent::peer_answer && a && b && (b->domain&(1u<<input.a))) {
        Word op=meaning::word(*b,input.other),own=meaning::word(*a,input.other);
        if (op<concept_words && own<concept_words) {
            auto response=meaning::infer(*b,op,0,{1u<<input.a,0});
            result.sound=meaning::pronounce(*b,response);Word heard=meaning::noun(*a,result.sound);
            if (meaning::unique(heard)) result.handled=meaning::point_view(*a,input.form,own,input.a,Word(__builtin_ctz(heard)));
        }
    } else for (Word role=0;role<2;++role) {
        auto *c=role ? b : a;if (!c) continue;Word visible=input.eyes&(1u<<role);bool other=(input.eyes&(1u<<(1-role)))!=0;
        bool accepted=false;
        if (k==ConceptEvent::board) {meaning::arrange(*c,input.a,input.b!=0);if (input.b) c->pending={};accepted=true;}
        if (k==ConceptEvent::verdict && input.a<3) {c->verdicts[input.a]=input.form;accepted=true;}
        if (k==ConceptEvent::name && visible) accepted=meaning::point(*c,input.form,input.a);
        if (k==ConceptEvent::link && input.a<32 && input.r<concept_channels && (c->domain&(1u<<input.a)) &&
            (input.b==empty_link || (input.b<32 && (c->domain&(1u<<input.b))))) {
            meaning::observe(*c,input.r,input.a,visible ? input.b : hidden_link,other);accepted=true;
        }
        if (k==ConceptEvent::sheet && input.r<concept_channels && input.a==Word(__builtin_popcount(c->domain))) {
            accepted=true;
            for (Word x=0;x<input.a;++x) accepted&=input.targets[x]==empty_link || input.targets[x]<input.a;
            if (accepted) for (Word x=0;x<input.a;++x) meaning::observe(*c,input.r,x,visible ? input.targets[x] : hidden_link,other);
        }
        if (k==ConceptEvent::example && visible) accepted=meaning::demonstrate(*c,input.form,input.a,input.b);
        if (k==ConceptEvent::scope && visible) {
            Word j=meaning::word(*c,input.other);accepted=meaning::point_view(*c,input.form,j,input.a,input.b);
        }
        result.handled|=Word(accepted)<<role;
    }
    *output=result;
}
KERNEL void concept_speech(Concepts *witness,Concepts *peer,const Count *tokens,Word n,Word statement,Word shared,ConceptReply *out) {
    if (index() || !witness) return;
    *out=meaning::converse(*witness,tokens,n,statement!=0,shared!=0);
    if (peer && statement && shared) meaning::converse(*peer,tokens,n,true,true);
}
KERNEL void voice_experiment(Bindings *b,const Recollection *m,Word form,Word offered,Word seed,Word random,Word *selected) {
    if (index()) return;
    Word k=binding::field(*b,form,false),serial=((b->attempt>>4)&0x07ffffff)+1,key=mix(seed^form^serial*0x682e328bu);
    Word chosen=k<4 ? binding::try_scene(b->fields[k],m->genes,b->genes,offered,key,random!=0) : key&3;
    b->attempted_form=form;b->attempt=0x80000000u|((serial&0x07ffffff)<<4)|((offered>>(4*chosen))&15);
    *selected=chosen;
}
KERNEL void voice_arrival(Bindings *b,const Recollection *m,Word after,Word credit,Word *accepted,Conjunctions *joint) {
    if (index()) return;
    *accepted=b->attempt>>31;
    if (*accepted) {
        conjunction::experience(*b,joint,m->genes,b->attempted_form,b->attempt&15,after,credit!=0);
        b->attempt&=0x7fffffffu;
    }
}

KERNEL void voice_reduce(const DialogueScore *scores,Word n,SpeechBlock *blocks) {
    Word i=index(),lane=i&127;
    __attribute__((shared)) static SpeechBlock group[128];
    SpeechBlock local{};
    if (i<n) {
        const auto &r=scores[i];
        local={{r.correct,r.attempts,r.voiced,r.moved,Word(r.correct==r.attempts),r.relevant_gazes,r.relative,r.relative_correct},0};
        Word rows[4]{},columns[5]{},calls=0;
        for (Word f=0;f<4;++f) for (Word v=0;v<5;++v) {
            Word count=r.calls[f][v];rows[f]+=count;columns[v]+=count;
        }
        for (Word count:rows) calls+=count;
        if (calls) for (Word f=0;f<4;++f) for (Word v=0;v<5;++v) {
            Word count=r.calls[f][v];if (!count) continue;
            float ratio=(float(count)*float(calls))/(float(rows[f])*float(columns[v])),bits;
            asm("lg2.approx.f32 %0, %1;":"=f"(bits):"f"(ratio));
            local.information+=(float(count)/float(calls))*bits;
        }
    }
    group[lane]=local;
    for (Word stride=64;stride;stride>>=1) {
        asm volatile("bar.sync 0;":::"memory");
        if (lane<stride) {
            for (Word j=0;j<8;++j) group[lane].counts[j]+=group[lane+stride].counts[j];
            group[lane].information+=group[lane+stride].information;
        }
    }
    if (!lane) blocks[i/128]=group[0];
}

KERNEL void voice_shuffle(OpticalMap *maps,Word count,Word seed,Word worlds,Word community) {
    Word id=index(),pair=id/32,lane=id&31;
    if (pair>=count) return;
    Word world=worlds ? pair%worlds : pair/community;
    for (Word role=0;role<2;++role) {
        Word key=mix(seed^world*0xaabd1e4bu^role*0x726c1c47u);
        Word value=mix(key+lane*0xf19b374du),rank=0;
        for (Word source=0;source<32;++source) {
            Word peer;
            asm volatile("shfl.sync.idx.b32 %0, %1, %2, 31, 0xffffffff;":"=r"(peer):"r"(value),"r"(source));
            rank+=peer<value;
        }
        for (Word bit=0;bit<5;++bit) {
            Word plane,active=(rank>>bit)&1;
            asm volatile("{ .reg .pred p; setp.ne.u32 p, %1, 0; vote.sync.ballot.b32 %0, p, 0xffffffff; }":"=r"(plane):"r"(active));
            if (!lane) maps[pair].planes[role][bit]=plane;
        }
    }
}
