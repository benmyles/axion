#pragma once
#include "composition_world.cuh"

namespace axion::life::composition {
struct Square {
    Phrase moved[8][4],ask[8],answer[8][4];
};
struct Enumeration {Word exact=0,answers=0,transitions=0,replies=0;};
AX_FN void prepare(Square &r,Mind &s,const shelter::Dialect &d) {
    for (Word a=0;a<8;++a) for (Word v=0;v<4;++v) {
        Count object=d.names[56+a],place=d.names[88+v];
        auto move=shelter::pronounce(d,d.input[shelter::put],object,place,0);
        auto ask=shelter::pronounce(d,d.input[shelter::where],object,0,0);
        r.moved[a][v]=perceive(s.lexicon,move,s.genes,true);r.ask[a]=perceive(s.lexicon,ask,s.genes,true);
    }
    for (Word a=0;a<8;++a) for (Word v=0;v<4;++v) {
        auto truth=shelter::pronounce(d,d.output[shelter::where],d.names[56+a],d.names[88+v],0);
        auto &expected=r.answer[a][v];expected={};expected.size=truth.size;
        // Require exact physical waves after all calibration. Recognition
        // tolerance must never turn an approximate vocalization into a pass.
        for (Word k=0;k<truth.size;++k) for (Word j=1;j<=s.lexicon.n;++j)
            if (s.lexicon.pulse[j]==truth.words[k]) {expected.words[k]=static_cast<Byte>(j);break;}
    }
}
AX_FN Enumeration enumerate(const Mind &m,const Square &r,Word arrangement) {
    Connections<32> state;Enumeration score;Word handled=0,depth=0,rotation=mix(arrangement)&7;
    for (Word k=0;k<8;++k) {Word a=(k+rotation)&7;act(m.anatomy,state,m.genes,r.moved[a][(arrangement>>(2*a))&3],handled,depth);}
    for (Word a=0;a<8;++a) {
        Word v=(arrangement>>(2*a))&3;score.answers+=equal(act(m.anatomy,state,m.genes,r.ask[a],handled,depth),r.answer[a][v]);
    }
    score.exact=score.answers==8;
    for (Word j=0;j<8;++j) {
        for (Word v=0;v<4;++v) {
            act(m.anatomy,state,m.genes,r.moved[j][v],handled,depth);Word exact=0;
            for (Word k=0;k<8;++k) {
                Word a=(k+j)&7,desired=a==j ? v : ((arrangement>>(2*a))&3);
                exact+=equal(act(m.anatomy,state,m.genes,r.ask[a],handled,depth),r.answer[a][desired]);
            }
            score.replies+=exact;score.transitions+=exact==8;
        }
        act(m.anatomy,state,m.genes,r.moved[j][(arrangement>>(2*j))&3],handled,depth);
    }
    return score;
}
}
