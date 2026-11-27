#pragma once
#include "sequence.cuh"

namespace axion::life::sequence {
constexpr Word horizon=48,query_length=sequence_cells+horizon+1;
struct Experiment {
    unsigned char actions[query_length]{};
    Word length=0,prefix=0;
};
struct Fork {
    unsigned char suffix[horizon]{},length=0,expected=0,yes=0,no=0;
};
enum class Phase : Word { fill,transfer,explore,contrast,old_check,new_check,repair,done };
struct Inquiry {
    Fork tests[sequence_cells-1]{};
    unsigned char parent[sequence_cells]{},approach[sequence_cells]{};
    unsigned char path[horizon]{},route[horizon+1]{};
    Phase phase=Phase::fill;
    Word root=0,node=0,source=0,motion=0,forks=0,focus=0,length=0,lo=0,hi=0,lower=0,upper=0;
    Word cursor=0,successive=0,questions=0,steps=0,challenged=0,confirmed=0,rejected=0,seed=0,repeat=0;
    Word patience=64,budget=4096,extent=16,limit=sequence_cells;
};
AX_FN void access(const Inquiry &b,Word state,Experiment &q) {
    Word n=0;
    while (state && n<sequence_cells) {q.actions[n++]=b.approach[state];state=b.parent[state];}
    for (Word j=0;j<n/2;++j) {auto x=q.actions[j];q.actions[j]=q.actions[n-1-j];q.actions[n-1-j]=x;}
    q.prefix=q.length=n;
}
AX_FN void suffix(Experiment &q,const unsigned char *word,Word n) {
    for (Word j=0;j<n;++j) q.actions[q.length++]=word[j];
}
AX_FN void start(Inquiry &b,SequenceMind &m,Word seed,Word trials,Word queries,Word span,Word cells) {
    b={};b.seed=seed;b.patience=trials;b.budget=queries;b.extent=span;b.limit=cells;
    m.map={};m.map.size=1;m.active=1;m.anchored=1;m.age=0;
}
AX_FN void next_edge(Inquiry &b,const SequenceMind &m) {
    if (++b.motion==m.motors) {b.motion=0;++b.source;}
    if (b.source==m.map.size) {b.phase=Phase::explore;b.successive=0;}
    b.node=b.root;
}
AX_FN void install(Inquiry &b,SequenceMind &m) {
    Word old=b.route[b.hi],fresh=m.map.size,branch=b.forks;
    auto &f=b.tests[branch];
    f.length=static_cast<unsigned char>(b.length-b.hi);
    for (Word j=0;j<f.length;++j) f.suffix[j]=b.path[b.hi+j];
    f.expected=static_cast<unsigned char>(b.upper);f.yes=static_cast<unsigned char>(old);f.no=static_cast<unsigned char>(fresh);
    if (b.root==old) b.root=sequence_cells+branch;
    else for (Word i=0;i<branch;++i) {
        auto &node=b.tests[i];if (node.yes==old) node.yes=static_cast<unsigned char>(sequence_cells+branch);
        if (node.no==old) node.no=static_cast<unsigned char>(sequence_cells+branch);
    }
    b.parent[fresh]=b.route[b.lo];b.approach[fresh]=b.path[b.lo];
    ++b.forks;++m.map.size;++m.edits;++b.confirmed;b.cursor=0;b.focus=old;b.phase=Phase::transfer;
}
AX_FN bool propose(Inquiry &b,SequenceMind &m,Experiment &q) {
    q.length=q.prefix=0;
    if (!m.motors || b.questions>=b.budget || b.successive>=b.patience || b.phase==Phase::done) {b.phase=Phase::done;return false;}
    if (b.phase==Phase::transfer) {
        Word bound=(m.map.size-1)*m.motors;
        while (b.cursor<bound) {
            Word e=m.map.edge[b.cursor/m.motors][b.cursor%m.motors];
            if (e && cell_of(e)==b.focus) break;
            ++b.cursor;
        }
        if (b.cursor==bound) {b.phase=Phase::fill;b.source=m.map.size-1;b.motion=0;b.node=b.root;}
    }
    switch (b.phase) {
    case Phase::fill:
        access(b,b.source,q);q.actions[q.length++]=static_cast<unsigned char>(b.motion);
        if (b.node>=sequence_cells) {auto &f=b.tests[b.node-sequence_cells];suffix(q,f.suffix,f.length);}
        break;
    case Phase::transfer: {
        access(b,b.cursor/m.motors,q);q.actions[q.length++]=static_cast<unsigned char>(b.cursor%m.motors);
        auto &f=b.tests[b.forks-1];suffix(q,f.suffix,f.length);break;
    }
    case Phase::explore: {
        Word key=mix(b.seed^m.genes.expression^b.questions*0x9e3779b9u),state=key%m.map.size;access(b,state,q);b.route[0]=static_cast<unsigned char>(state);
        Word motif=(key>>16)&7,period=1+((key>>20)&3);
        b.length=1+mix(key+17)%b.extent;
        // Interleave babbling with the learner's own distinguishing experiences.
        // A rare audible consequence can now be elicited after a new arrival.
        Word suffix_length=0;const unsigned char *ending=nullptr;
        if (b.forks && motif>=4) {
            const auto &test=b.tests[mix(key+43)%b.forks];ending=test.suffix;suffix_length=test.length;
            if (suffix_length>=b.extent) suffix_length=b.extent-1;
            Word space=b.extent-suffix_length;
            b.length=1+mix(key+73)%(space<5 ? space : 5);
        }
        for (Word j=0;j<b.length;++j) {
            Word action=motif==0 ? ((key>>8)%m.motors) : motif==1 && j>=period ? b.path[j-period] : mix(key+(j+1)*0x941ac27du)%m.motors;
            b.path[j]=static_cast<unsigned char>(action);
        }
        for (Word j=0;j<suffix_length;++j) b.path[b.length++]=ending[j];
        for (Word j=0;j<b.length;++j) {
            q.actions[q.length++]=b.path[j];state=cell_of(m.map.edge[state][b.path[j]]);
            b.route[j+1]=static_cast<unsigned char>(state);
        }
        break;
    }
    case Phase::contrast: {
        Word middle=(b.lo+b.hi)/2;access(b,b.route[middle],q);suffix(q,b.path+middle,b.length-middle);break;
    }
    case Phase::old_check:case Phase::repair:
        access(b,b.route[b.hi],q);suffix(q,b.path+b.hi,b.length-b.hi);break;
    case Phase::new_check:
        access(b,b.route[b.lo],q);suffix(q,b.path+b.lo,b.length-b.lo);break;
    default:return false;
    }
    ++b.questions;b.steps+=q.length+1;return true;
}
AX_FN void receive(Inquiry &b,SequenceMind &m,const Experiment &q,const unsigned char *observed) {
    Word last=observed[q.length-1];
    if (last>=sequence_senses) {++b.rejected;if (b.phase==Phase::explore) b.successive=0;return;}
    switch (b.phase) {
    case Phase::fill: {
        Word s=observed[q.prefix];if (s>=sequence_senses) return;
        if (b.node>=sequence_cells) {auto &fork=b.tests[b.node-sequence_cells];b.node=last==fork.expected ? fork.yes : fork.no;}
        if (b.node<sequence_cells) {m.map.edge[b.source][b.motion]=contact(b.node,s);next_edge(b,m);}
        break;
    }
    case Phase::transfer: {
        auto &e=m.map.edge[b.cursor/m.motors][b.cursor%m.motors];
        auto &f=b.tests[b.forks-1];e=contact(last==f.expected ? f.yes : f.no,sense_of(e));++b.cursor;break;
    }
    case Phase::explore: {
        Word j=0;
        while (j<b.length && observed[q.prefix+j]==sense_of(m.map.edge[b.route[j]][b.path[j]])) ++j;
        if (j==b.length) {++b.successive;break;}
        ++b.challenged;b.successive=0;
        if (observed[q.prefix+j]>=sequence_senses) break;
        b.length=j+1;b.lo=0;b.hi=j;b.lower=observed[q.prefix+j];b.upper=sense_of(m.map.edge[b.route[j]][b.path[j]]);b.repeat=0;
        b.phase=!j ? Phase::repair : j==1 ? Phase::old_check : Phase::contrast;break;
    }
    case Phase::contrast: {
        Word mid=(b.lo+b.hi)/2;
        if (last==b.lower) b.lo=mid;else {b.hi=mid;b.upper=last;}
        if (b.hi==b.lo+1) b.phase=Phase::old_check;
        break;
    }
    case Phase::old_check:
        if (last!=b.upper) {b.phase=Phase::explore;++b.rejected;}else b.phase=Phase::new_check;
        break;
    case Phase::new_check: {
        bool stable=last==b.lower && last!=b.upper;
        Word cue=Word(stable)+2*Word(b.repeat!=0)+4*Word(b.length>2);
        if (!stable || !((m.genes.high>>(8+cue))&1)) {++b.rejected;b.phase=Phase::explore;break;}
        if (++b.repeat<=((m.genes.expression>>12)&1)) {b.phase=Phase::old_check;break;}
        if (m.map.size>=b.limit) {b.phase=Phase::done;break;}
        install(b,m);break;
    }
    case Phase::repair: {
        if (last==b.lower && ((m.genes.high>>20)&1)) {
            auto &e=m.map.edge[b.route[0]][b.path[0]];e=contact(cell_of(e),last);
        }
        ++b.rejected;b.phase=Phase::explore;break;
    }
    default:break;
    }
}
}
