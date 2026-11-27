#pragma once
#include "report.hpp"

namespace axion::life {
constexpr Word concept_nodes=32,concept_channels=4,concept_words=8,concept_scopes=2;
constexpr Word empty_link=32,hidden_link=33;
struct Fibres {Word target[5]{},seen=0,connected=0;};
struct RelationalView {Fibres relation[concept_channels]{};};
struct RouteWord {
    Count form=0,walk=0;
    Word low[8]{},high[8]{},inputs=0,threshold=0;
};
struct Perspective {Count form=0;Word strength=0,observations=0;};
struct ProbeTrace {
    Count form=0;
    RelationalView scene{};
    Count prior[7]{};
    Word source=0,domain=0;
};
struct Concepts {
    RecollectionGenes genes{0,0,0};
    Word domain=0;
    RelationalView views[2]{};
    Count nouns[concept_nodes]{};
    Word referents[concept_nodes]{};
    RouteWord words[concept_words]{};
    Perspective scopes[concept_scopes]{};
    Count verdicts[3]{};
    ProbeTrace pending{};
    Count questions=0;
};
struct Activation {Word cells=0,gap=0;};
struct EndpointProjection {Word labels[5],grounded;};
static_assert(sizeof(EndpointProjection)==24);
AX_FN Word read_endpoint(const EndpointProjection &p,Word x) {
    if (!(p.grounded&(1u<<x))) return hidden_link;
    Word y=0;for (Word bit=0;bit<5;++bit) y|=((p.labels[bit]>>x)&1)<<bit;
    return y;
}
struct ConceptReply {
    Count sound=0,question=0,subject=0;
    Word handled=0,cells=0,gap=0,start=0,context=0,steps=0,hops=0,trace[9]{};
};
enum class ConceptEvent : Word { board,name,link,sheet,example,scope,peer_answer,verdict,ask,reply,cancel };
struct ConceptInput {Count form=0,other=0;ConceptEvent event=ConceptEvent::board;Word a=0,b=0,r=0,eyes=3;unsigned char targets[32]{};};
static_assert(sizeof(Fibres)==28 && sizeof(RouteWord)==88 && sizeof(ProbeTrace)==184 && sizeof(Concepts)==1576);
#if !defined(__CUDA__)
int concept_command(int argc,char **argv);
#endif
}
