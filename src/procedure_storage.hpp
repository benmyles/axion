#pragma once
#include "axion/procedure.hpp"
#include <stdexcept>

namespace axion::life::procedure {
inline void validate(const Mind &m) {
    auto invalid=[](){throw std::runtime_error("invalid procedural circuit");};
    const auto &p=m.circuit;const auto &b=m.memory;
    if (p.guard_n>guard_limit || m.retina>255 || m.observing>1 || m.ear.size>letters || p.count>productions || p.bytes>code_bytes || b.count>lessons || b.bytes>experience_bytes ||
        b.count>b.observed || m.consolidated>b.observed || m.extent!=sizeof(Mind)) invalid();
    for (Byte k:m.ear.cache) if (k>m.ear.size) invalid();
    if (m.muscle.forms>effectors || m.muscle.first>7 || m.muscle.second>7) invalid();
    if (m.recursion.n>loops) invalid();
    for (Word k=0;k<m.recursion.n;++k) {
        const auto &r=m.recursion.state[k];
        if (r.n<2 || r.n>12 || r.head+r.tail==0 || r.head>motif || r.tail>motif || r.center>24 || r.bytes>loop_bytes) invalid();
        for (Word i=0;i<r.head;++i) if (!r.prefix[i] || r.prefix[i]>m.ear.size) invalid();
        for (Word i=0;i<r.tail;++i) if (!r.suffix[i] || r.suffix[i]>m.ear.size) invalid();
        for (Word i=0;i<r.center;++i) if (!r.base[i] || r.base[i]>m.ear.size) invalid();
        Word off=0;
        for (Word j=0;j<r.n;++j) {
            auto a=r.arm[j];
            if (a.at!=off || !a.n || a.n>16 || a.m>24 || a.m<r.center || !a.cuts || (a.cuts>>(a.m-r.center+1)) || off+a.n+a.m>r.bytes) invalid();
            for (Word v=0;v<a.n+a.m;++v) if (!r.wire[off+v] || r.wire[off+v]>m.ear.size) invalid();
            for (Word bits=a.cuts;bits;bits&=bits-1) if (!same({r.wire+off+a.n+__builtin_ctz(bits),r.center},{r.base,r.center})) invalid();
            off+=a.n+a.m;
        }
        if (off!=r.bytes) invalid();
    }
    Word cursor=0;
    for (Word i=0;i<p.count;++i) {
        auto c=p.cell[i];if (c.at!=cursor || !c.input || c.input>motif || c.output>motif || c.arity>2 ||
            (c.raw>>c.arity) || cursor+c.input+c.output>p.bytes) invalid();
        Word inputs=0,outputs=0;
        for (Word j=0;j<c.input;++j) {
            Byte op=p.wire[cursor++];
            if (op==call0+inputs && inputs<c.arity) ++inputs;
            else if (!op || op>m.ear.size) invalid();
        }
        if (inputs!=c.arity || c.input<=c.arity) invalid();
        for (Word j=0;j<c.output;++j) {
            Byte op=p.wire[cursor++];
            if (var(op)) {
                Word k=slot(op);
                if (op>quote1 || k>=c.arity || bool(c.raw&(1u<<k))!=(op>=quote0)) invalid();
                outputs|=1u<<k;
            } else if (!op || op>m.ear.size) invalid();
        }
        Word guard=c.flags>>2;
        if (guard>p.guard_n) invalid();
        if (guard) {
            const auto &g=p.guards[guard-1];if (g.length>motif) invalid();
            for (Word k=0;k<8;++k) if (g.second[k]&~g.known[k]) invalid();
            for (Word k=0;k<g.length;++k) {Byte v=g.effect[k];
                if (var(v)) {if (v>quote1 || slot(v)>=c.arity || bool(c.raw&(1u<<slot(v)))!=(v>=quote0)) invalid();outputs|=1u<<slot(v);}
                else if (!v || v>m.ear.size) invalid();
            }
        }
        if (outputs!=(1u<<c.arity)-1) invalid();
    }
    if (cursor!=p.bytes) invalid();
    cursor=0;for (Word i=0;i<b.count;++i) {auto r=b.record[i];if (r.at!=cursor || !r.before) invalid();cursor+=r.before+r.after;}
    if (cursor!=b.bytes) invalid();
    for (Word j=0;j<b.count;++j) if (b.visible[j]>1) invalid();
    const auto &work=m.performance;
    if (work.active>1 || work.size>performance_span || work.depth>performance_depth || work.nodes>performance_span ||
        (work.active && (!work.size || !work.depth || work.revision!=m.revisions))) invalid();
    for (Word j=0;j<work.size;++j) if (!work.input[j] || work.input[j]>m.ear.size) invalid();
    for (Word j=0;j<work.depth;++j) if (!work.stack[j].node || work.stack[j].node>work.nodes || work.stack[j].branch>2 ||
        work.stack[j].pc>motif || work.stack[j].offset>performance_span) invalid();
    for (Word i=0;i<b.bytes;++i) if (!b.tape[i] || b.tape[i]>m.ear.size) invalid();
}
}
