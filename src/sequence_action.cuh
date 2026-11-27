#pragma once
#include "sequence.cuh"

namespace axion::life::sequence {
// The target is an observed waveform. No state carries innate food, place,
// question or answer semantics. Trace the first gesture of a shortest path.
AX_FN Word approach(const SequenceMind &m,Word start,Word sense) {
    unsigned char queue[sequence_cells],first[sequence_cells];
    Count seen=1ull<<start;queue[0]=static_cast<unsigned char>(start);first[0]=sequence_motors;
    Word tail=1;
    for (Word i=0;i<tail;++i) {
        for (Word a=0;a<m.motors;++a) {
            Word e=m.map.edge[queue[i]][a];if (!e) continue;
            Word initial=i ? first[i] : a;
            if (sense_of(e)==sense) return initial;
            Word to=cell_of(e);Count bit=1ull<<to;
            if (!(seen&bit)) {seen|=bit;queue[tail]=static_cast<unsigned char>(to);first[tail++]=static_cast<unsigned char>(initial);}
        }
    }
    return sequence_motors;
}
AX_FN Word pursue_wave(const SequenceMind &m,Count target) {
    if (!m.active) return sequence_motors;
    Word sense=sequence_senses;
    for (Word i=0;i<m.senses;++i) if (m.retina[i].high==target) {sense=i;break;}
    if (sense==sequence_senses) return sequence_motors;
    Count states=m.active;Word chosen=sequence_motors;
    while (states) {
        Word q=__builtin_ctzll(states);states&=states-1;
        Word gesture=approach(m,q,sense);
        if (gesture==sequence_motors || (chosen<sequence_motors && chosen!=gesture)) return sequence_motors;
        chosen=gesture;
    }
    return chosen;
}
}
