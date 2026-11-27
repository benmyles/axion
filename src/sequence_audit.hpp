#pragma once
#include "axion/sequence.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <queue>
#include <vector>

namespace axion::life::sequence_audit {
// This independent verifier runs after learning. A breadth-first search
// of the synchronous product proves agreement on every finite word from home.
struct Certificate {
    Word correct=0,tests=0,states=0,minimal=0,bits=0;
    std::vector<Word> witness;
    bool exact() const {return tests && tests==correct;}
};
inline Certificate check(const Chamber &w,const SequenceMind &m) {
    Certificate c;
    std::array<Word,sequence_cells> classes{},next{};
    for (;;) {
        Word groups=0;
        for (Word i=0;i<w.states;++i) {
            Word j=0;
            for (;j<i;++j) {
                bool equal=true;
                for (Word a=0;a<w.motors;++a) {
                    Word x=w.rule[i][a],y=w.rule[j][a];
                    if (w.sounds[sense_of(x)]!=w.sounds[sense_of(y)] || classes[cell_of(x)]!=classes[cell_of(y)]) {equal=false;break;}
                }
                if (equal) break;
            }
            next[i]=j==i ? groups++ : next[j];
        }
        if (next==classes) break;
        classes=next;
    }
    std::array<bool,sequence_cells> reachable{},seen_class{};
    std::queue<Word> search;search.push(w.origin);reachable[w.origin]=true;
    while (!search.empty()) {
        Word q=search.front();search.pop();seen_class[classes[q]]=true;
        for (Word a=0;a<w.motors;++a) {auto to=cell_of(w.rule[q][a]);if (!reachable[to]) {reachable[to]=true;search.push(to);}}
    }
    c.minimal=std::count(seen_class.begin(),seen_class.end(),true);
    constexpr Word pairs=sequence_cells*sequence_cells,none=~0u;
    std::array<Word,pairs> parent,action{};parent.fill(none);
    Word start=w.origin*sequence_cells;parent[start]=start;search.push(start);
    while (!search.empty()) {
        Word pair=search.front();search.pop();++c.states;
        for (Word a=0;a<w.motors;++a) {
            Word x=w.rule[pair/sequence_cells][a],y=m.map.edge[pair%sequence_cells][a];
            Word errors=y ? std::popcount(w.sounds[sense_of(x)]^m.retina[sense_of(y)].high) : 64;
            ++c.tests;c.bits+=errors;c.correct+=errors==0;
            if (errors && c.witness.empty()) {
                c.witness.push_back(a);
                for (Word k=pair;k!=start;k=parent[k]) c.witness.push_back(action[k]);
                std::reverse(c.witness.begin(),c.witness.end());
            }
            if (!y) continue;
            Word to=cell_of(x)*sequence_cells+cell_of(y);
            if (parent[to]==none) {parent[to]=pair;action[to]=a;search.push(to);}
        }
    }
    return c;
}
}
