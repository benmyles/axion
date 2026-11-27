#pragma once
#include "axion/sequence.hpp"

namespace axion::life::sequence {
AX_FN Word pop(Count x) {return Word(__builtin_popcountll(x));}
AX_FN Count all(Word n) {return n>=64 ? ~0ull : (1ull<<n)-1;}
AX_FN void adapt(Sensation &s,Count signal,Count visible,RecollectionGenes genes) {
    Count lo=0,hi=0;
    for (Word y=0;y<2;++y) {
        Count active=y ? signal : ~signal;
        for (Word old=0;old<4;++old) {
            Count match=(old&1 ? s.low : ~s.low)&(old&2 ? s.high : ~s.high)&active;
            Word next=(genes.low>>(2*(4*y+old)))&3;if (next&1) lo|=match;if (next&2) hi|=match;
        }
    }
    s.low=(s.low&~visible)|(lo&visible);s.high=(s.high&~visible)|(hi&visible);
}
AX_FN Word recognize(SequenceMind &m,Count signal,Count visible,bool plastic) {
    if (!visible) return sequence_senses;
    Word best=~0u,pick=sequence_senses,threshold=m.genes.high&31;
    for (Word j=0;j<m.senses;++j) {
        const auto &s=m.retina[j];Count diff=(s.high^signal)&visible;
        Word distance=pop(diff);if (m.genes.high&32) distance+=pop(diff&~(s.high^s.low));
        if (distance<best) {best=distance;pick=j;}
    }
    // Normalise to visible channels. A shuttered receptor is never evidence
    // that its opposite polarity fired.
    if (pick>=sequence_senses || 64*best>threshold*pop(visible)) {
        if (!plastic || m.senses==sequence_senses) return sequence_senses;
        pick=m.senses++;m.retina[pick]={m.genes.high&64 ? signal&visible : 0,signal&visible};return pick;
    }
    if (plastic) adapt(m.retina[pick],signal,visible,m.genes);
    return pick;
}
AX_FN Word motor(SequenceMind &m,Count gesture,bool recruit=false) {
    for (Word j=0;j<m.motors;++j) if (m.gestures[j]==gesture) return j;
    if (!recruit || m.motors>=sequence_motors) return sequence_motors;
    Word j=m.motors++;m.gestures[j]=gesture;return j;
}
AX_FN void record(RetainedTrace &t,Word a,Word o,bool entry) {
    Word i=t.head;t.senses[i]=static_cast<unsigned char>(o|(entry ? 128 : 0));
    Word shift=4*(i&7);t.motors[i/8]=(t.motors[i/8]&~(15u<<shift))|(a<<shift);
    t.head=(i+1)&(trace_slots-1);if (t.length<trace_slots) ++t.length;
}
AX_FN Word at(const RetainedTrace &t,Word ordinal) {return (t.head+trace_slots-t.length+ordinal)&(trace_slots-1);}
AX_FN Word gesture_at(const RetainedTrace &t,Word i) {return (t.motors[i/8]>>(4*(i&7)))&15;}
AX_FN SequenceEcho anticipate(const SequenceGraph &g,Count active,Word a) {
    SequenceEcho answer{};if (a>=sequence_motors) return answer;
    while (active) {
        Word q=Word(__builtin_ctzll(active));active&=active-1;Word arc=g.edge[q][a];
        if (arc) {answer.emissions|=1u<<sense_of(arc);answer.after|=1ull<<cell_of(arc);}
        else answer.missing=1;
    }
    return answer;
}
AX_FN Count track(const SequenceGraph &g,Count active,Word motion,Word observation) {
    Count destination=0;
    while (active) {
        Word q=Word(__builtin_ctzll(active));active&=active-1;Word arc=g.edge[q][motion];
        if (arc && sense_of(arc)==observation) destination|=1ull<<cell_of(arc);
    }
    return destination;
}
AX_FN bool observe(SequenceMind &m,Word action,Word output,bool entry,bool plastic=true) {
    if (action>=m.motors || output>=m.senses) {
        if (plastic) m.trace.length=0;
        auto guess=anticipate(m.map,m.active,action);
        m.active=guess.missing || !guess.after ? all(m.map.size) : guess.after;
        ++m.events;++m.age;return false;
    }
    if (plastic) record(m.trace,action,output,entry);
    if (entry) m.active=m.anchored ? 1 : all(m.map.size);
    if (!m.active) m.active=all(m.map.size);
    auto forecast=anticipate(m.map,m.active,action);
    Count changed=track(m.map,m.active,action,output);
    bool mistaken=!(forecast.emissions&(1u<<output));m.errors+=mistaken;
    if (!changed && !entry) changed=track(m.map,all(m.map.size),action,output);
    m.active=changed;++m.events;++m.age;return mistaken;
}
}
