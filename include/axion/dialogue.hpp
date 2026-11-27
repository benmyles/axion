#pragma once
#include "actor.hpp"
#include "actor_batch.hpp"
#include "auditory.hpp"
#include "attention.hpp"
#include "binding.hpp"
#include "recollection.hpp"
#include "report.hpp"
#include "causal.hpp"

namespace axion::life {
AX_FN Word sensory_address(Word key,Word tag) {return 1u<<((tag*((key>>8)|1)+key)&31);}
enum class Wire : Word { open, silent, shifted, hidden, no_question, shift_question, stranger, blind_gaze, muted_self, shifted_self };
struct OpticalMap { Word planes[2][5]; };
struct ImaginedWord { Count image=0,visual=0,paths=0;Word first=0;Count bindings=0,roles=0,joint=0,scenes=0,reporter=0,audience=0; };
struct DialogueConfig { Word pairs,seed,facets,reversal,learning,plasticity,worlds,community,active_vision,compass,nouns,withheld,discourse; Wire wire;
    Word distributed=0,forms[4]{};Count optics=0;Word outcomes=0;Count recall=0;Word recall_cut=0;
    Count caller=0,iris=0;Word pitch_cut=0;Count projection=0,bindings=0,roles=0,joint=0,scenes=0; };
struct VoiceGermline { Propagule body;Word auditory_rule;RecollectionGenes recall;Word attention_rule,iris_rule;BindingGenes binding;RecollectionGenes gates{0,0,0},report{0,0,0},concepts{0,0,0},agency{0,0,0},sequence{0,0,0},composition{0,0,0},procedure{0,0,0},grounding{0,0,0}; };
struct VoiceBody {
    ActorBatch shape;
    Word tiles,bits,delayed,outcome_lag,dendrites,relations,alternative,balanced,navigation,auditory,recollects,attends,orients,binds,conjoins,reports,conceptual,causal,sequential,composes,procedural,grounded;
};
struct Exchange {
    Word round,shape,colour,topic,question,voice,looked,form,self;
    Word shutters,dim,retinal,attention;
};
struct DialogueScore {
    Word attempts,correct,voiced,moved,relevant_gazes,relative,relative_correct;
    Word calls[4][5],requests[2][5];
};
struct SpeechBlock { Word counts[8];float information; };
#if !defined(__CUDA__)
int dialogue_command(int argc,char **argv);
#endif
}
