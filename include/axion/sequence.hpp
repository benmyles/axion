#pragma once
#include "recollection.hpp"
#include "life.hpp"

namespace axion::life {
constexpr Word sequence_cells=64,sequence_motors=16,sequence_senses=32,trace_slots=256;
struct Sensation {Count low=0,high=0;};
struct SequenceGraph {
    unsigned short edge[sequence_cells][sequence_motors]{};
    Word size=0;
};
struct RetainedTrace {
    unsigned char senses[trace_slots]{};
    Word motors[trace_slots/8]{};
    Word head=0,length=0;
};
// The channels carry physical pulse patterns. Neither table contains
// hidden-state identities, grammar roles or a prescribed interpretation.
struct SequenceMind {
    RecollectionGenes genes{};
    Count gestures[sequence_motors]{};
    Sensation retina[sequence_senses]{};
    SequenceGraph map{};
    RetainedTrace trace{};
    Count active=0;
    Word motors=0,senses=0,events=0,errors=0,edits=0,age=0,anchored=0;
};
struct SequenceEcho {Word emissions=0,missing=0;Count after=0;};
struct Chamber {
    unsigned short rule[sequence_cells][sequence_motors]{};
    Count sounds[sequence_senses]{},gestures[sequence_motors]{};
    Word states=1,motors=2,senses=2,origin=0;
};
AX_FN unsigned short contact(Word cell,Word sense) {return static_cast<unsigned short>(1+cell+(sense<<6));}
AX_FN Word cell_of(Word contact) {return (contact-1)&63;}
AX_FN Word sense_of(Word contact) {return (contact-1)>>6;}
static_assert(sizeof(SequenceMind)==3144);
static_assert(sizeof(Sensation)==16 && sizeof(RetainedTrace)==392);
#if !defined(__CUDA__)
int develop_command(int argc,char **argv);
#endif
}

namespace axion::life {
struct DevelopmentLimits {
    Word patience=64,queries=2048,span=24,cells=sequence_cells;
};
struct DevelopmentCost {Word trips=0,moves=0,contrasts=0,splits=0,rejected=0;};
struct PulsePort {
    Count gesture=0,sensation=0,visible=~0ull,heard=0,active=0;
    Word kind=0,home=0,answered=0,options=0;
};
enum PortKind : Word { predict, demonstrate, return_home, lose_position, pursue };
}
