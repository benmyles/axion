#pragma once
#include "concept.hpp"

namespace axion::life {
constexpr Word traits=6,operations=6,local_states=1u<<traits,all_traits=local_states-1;
constexpr Word pairs_of_traits=15,all_pairs=(1u<<pairs_of_traits)-1;
constexpr Word utterance_limit=64,no_operation=operations;
// Each of the 15 dendrites samples a different pair of afferents. Its four
// possible input patterns have two independently writable synaptic bits.
struct CausalField {
    unsigned short low[4]{},high[4]{},viable=0,compiled=0;
};
struct CausalStep {Word on=0,unknown=0;};
struct CausalModel {
    RecollectionGenes genes{};
    CausalField fields[operations][traits]{};
    Count visited[operations]{};
};
struct GroundedSignals {Count forms[32]{};Word meanings[32]{};};
enum class Disposition : Word { plan=18,forecast,why,act,describe,taste,forage };
struct ExplanationFrame {
    Count fixed[8]{};unsigned char roles[8]{},length=0;
};
struct Intention {
    Word object=32,start=0,require=0,value=0,length=0,branch=0;
    unsigned char actions[utterance_limit]{};
};
struct Appetite {CausalField field{};Word bites=0;Count sampled=0;};
struct GoalWord {Count form=0;CausalField field{};Word lessons=0;};
struct Branch {Word sense=traits,low=no_operation,high=no_operation;};
struct GoalEvidence {Count low=0,high=0,certain=0,possible=0;};
struct GoalAssembly {GoalEvidence field[8]{};Word active=0;unsigned char inputs[8]{};};
struct Agency {
    CausalModel model{};
    GroundedSignals speech{};
    ExplanationFrame explanation{};
    Word observed=0;
    unsigned char scenes[32]{};
    Intention intention{};
    Word alignment=0;
    Appetite appetite{};
    GoalWord aims[8]{};
    ExplanationFrame conditional{};
    Count desired=0,goal_name=0;
    Branch branch{};
    Word spare=0;
    GoalAssembly assembly{};
};
struct Apparatus {
    unsigned short gates[operations][traits]{};
    unsigned char plots[32]{};
    Word present=0;
    Word food=0,dormant=0;
};
struct Intervention {Word before=0,action=0,after=0,known_before=all_traits,known_after=all_traits;};
struct CausalAudit {Word forecasts=0,reached=0,length=0,reason=0,unreachable=0,decided=0,compiled=0,necessary=0;};
enum class GardenEvent : Word { open,place,transition,contact,vocabulary,change,experience,probe,imitate,speech,inspect,relay,diet,taste,category,branch,contrast,survey,lantern };
struct GardenPort {
    GardenEvent event=GardenEvent::speech;
    Word a=0,b=0,c=0,d=0,eyes=3,shared=1,length=0,enabled=63,known_before=63,known_after=63,reuse=1;
};
struct GardenReply {
    Count sounds[utterance_limit]{};
    Word count=0,handled=0,kind=0,subject=32,before=0,after=0,require=0,value=0,actions=0,solved=0,branch=0,repairs=0,values=0,selected=6;
    Word samples=0;unsigned char trajectory[utterance_limit+1]{};
};
static_assert(__builtin_offsetof(Agency,appetite)==1376 && __builtin_offsetof(Agency,aims)==1408 && __builtin_offsetof(Agency,assembly)==1776 && sizeof(Agency)==2048);
static_assert(sizeof(CausalField)==20 && sizeof(CausalModel)==784 && sizeof(Intention)==88);
#if !defined(__CUDA__)
int cultivate_command(int argc,char **argv);
#endif
}
