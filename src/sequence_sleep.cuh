#pragma once
#include "sequence.cuh"

namespace axion::life::sequence {
// Temporary replay tissue is shared by the block and released after a fit.
// A prefix has at most one output and successor for each motor.
constexpr Word replay_nodes=trace_slots+1,move_log=trace_slots*10;
struct UnionEdit {unsigned short a,b;short old_a,old_b;};
struct ReplayTissue {
    unsigned short edges[replay_nodes][sequence_motors];
    short parent[replay_nodes];
    unsigned short red[sequence_cells],left[replay_nodes],right[replay_nodes],added[move_log];
    UnionEdit edits[replay_nodes];
    Word nodes,reds;
};
AX_FN void sync() {asm volatile("bar.warp.sync 0xffffffff;":::"memory");}
AX_FN Word vote(bool p) {Word bits;asm volatile("{ .reg .pred b; setp.ne.u32 b,%1,0; vote.sync.ballot.b32 %0,b,0xffffffff; }":"=r"(bits):"r"(Word(p)));return bits;}
AX_FN Word share(Word x,Word lane=0) {Word v;asm volatile("shfl.sync.idx.b32 %0,%1,%2,31,0xffffffff;":"=r"(v):"r"(x),"r"(lane));return v;}
AX_FN Word root(const ReplayTissue &w,Word i) {while (w.parent[i]>=0) i=Word(w.parent[i]);return i;}
AX_FN Word destination(Word link) {return (link-1)&511;}
AX_FN Word emission(Word link) {return (link-1)>>9;}
AX_FN Word replay_link(Word to,Word sense) {return 1+to+(sense<<9);}
AX_FN bool fold(ReplayTissue &w,Word first,Word second,Word motors,Word lane,Word &evidence,bool commit,Word seeds=0) {
    if (!seeds && !lane) {w.left[0]=static_cast<unsigned short>(first);w.right[0]=static_cast<unsigned short>(second);}sync();
    Word pending=seeds ? seeds : 1,moves=0,edits=0;bool consistent=true;evidence=0;
    while (pending) {
        --pending;Word a=root(w,w.left[pending]),b=root(w,w.right[pending]);if (a==b) continue;
        if (w.parent[b]<w.parent[a]) {Word t=a;a=b;b=t;}
        Word x=lane<motors ? w.edges[a][lane] : 0,y=lane<motors ? w.edges[b][lane] : 0;
        Word clashes=vote(x && y && emission(x)!=emission(y));
        if (clashes) {consistent=false;break;}
        Word same=vote(x && y),extra=vote(!x && y),forced=Word(__builtin_popcount(same)),attached=Word(__builtin_popcount(extra));
        if (pending+forced>replay_nodes || moves+attached>move_log || edits>=replay_nodes) {consistent=false;break;}
        Word preceding=(1u<<lane)-1;
        if (x && y) {
            Word i=pending+Word(__builtin_popcount(same&preceding));
            w.left[i]=static_cast<unsigned short>(destination(x));w.right[i]=static_cast<unsigned short>(destination(y));
        } else if (y) {
            Word i=moves+Word(__builtin_popcount(extra&preceding));
            w.added[i]=static_cast<unsigned short>(a*sequence_motors+lane);w.edges[a][lane]=static_cast<unsigned short>(y);
        }
        if (!lane) {w.edits[edits]={static_cast<unsigned short>(a),static_cast<unsigned short>(b),w.parent[a],w.parent[b]};
            w.parent[a]=static_cast<short>(w.parent[a]+w.parent[b]);w.parent[b]=static_cast<short>(a);}
        ++edits;pending+=forced;moves+=attached;evidence+=forced;sync();
    }
    if (!commit || !consistent) {
        for (Word i=lane;i<moves;i+=32) {Word slot=w.added[i];w.edges[slot/sequence_motors][slot%sequence_motors]=0;}
        if (!lane) while (edits) {auto e=w.edits[--edits];w.parent[e.a]=e.old_a;w.parent[e.b]=e.old_b;}
        sync();
    }
    return consistent;
}
// A recurrent sensory event sometimes localizes an animal without a home
// reset. Unify all its arrivals in one transaction. Any contradictory
// motor consequence anywhere in the closure cancels the entire experiment.
AX_FN Word arrivals(ReplayTissue &w,Word motors,Word cue,Word action,Word lane) {
    Word n=0;
    if (!lane) {
        Word first=~0u;
        for (Word q=0;q<w.nodes;++q) if (w.parent[q]<0) for (Word a=0;a<motors;++a) {
            Word e=w.edges[q][a];if (!e || emission(e)!=cue || (action<motors && a!=action)) continue;
            Word to=root(w,destination(e));
            if (first==~0u) first=to;
            else if (first!=to && n<replay_nodes) {
                w.left[n]=static_cast<unsigned short>(first);w.right[n++]=static_cast<unsigned short>(to);
            }
        }
    }
    return share(n);
}
AX_FN void anchor(ReplayTissue &w,Word motors,Word cue,Word action,Word lane,Word floor) {
    Word n=arrivals(w,motors,cue,action,lane),evidence=0;
    if (n<2 || !fold(w,0,0,motors,lane,evidence,false,n) || evidence<floor) return;
    // The trial reuses the queues for its transitive closure.
    n=arrivals(w,motors,cue,action,lane);
    fold(w,0,0,motors,lane,evidence,true,n);
}
AX_FN Word red_number(const ReplayTissue &w,Word representative) {
    for (Word j=0;j<w.reds;++j) if (root(w,w.red[j])==representative) return j;return sequence_cells;
}
AX_FN void sleep(SequenceMind &mind,ReplayTissue &w,Word lane,Word limit=sequence_cells) {
    auto &t=mind.trace;if (!t.length || !mind.motors) return;
    Word conflict=0;
    for (Word i=lane;i<replay_nodes*sequence_motors;i+=32) w.edges[i/sequence_motors][i%sequence_motors]=0;
    for (Word i=lane;i<replay_nodes;i+=32) w.parent[i]=-1;
    sync();Word pos=0,offset=0;
    // A truncated prefix preceding a known homecoming is a different history.
    if (!(t.senses[at(t,0)]&128)) {
        for (Word j=1;j<t.length;++j) if (t.senses[at(t,j)]&128) {offset=j;break;}
    }
    if (!lane) {
        w.nodes=1;w.reds=1;w.red[0]=0;
        for (Word j=offset;j<t.length;++j) {
            Word i=at(t,j),a=gesture_at(t,i),s=t.senses[i];if (s&128) pos=0;s&=31;
            Word e=w.edges[pos][a];
            if (e && emission(e)!=s) {conflict=1;break;}
            if (!e) {e=replay_link(w.nodes++,s);w.edges[pos][a]=static_cast<unsigned short>(e);}
            pos=destination(e);
        }
    }
    sync();if (share(conflict)) return;
    Word budget=1+((mind.genes.expression>>4)&7),floor=mind.genes.expression&3;
    if ((mind.genes.expression>>26)&3) {
        for (Word o=0;o<mind.senses;++o) anchor(w,mind.motors,o,mind.motors,lane,1+floor);
        if ((mind.genes.expression>>26)&2) for (Word a=0;a<mind.motors;++a)
            for (Word o=0;o<mind.senses;++o) anchor(w,mind.motors,o,a,lane,1+floor);
    }
    for (;;) {
        Word best=0,winner=~0u,model=0,promote=~0u,looked=0;bool stop=false;
        for (Word i=0;i<w.reds && !stop;++i) for (Word a=0;a<mind.motors;++a) {
            Word source=root(w,w.red[i]),edge=w.edges[source][a];if (!edge) continue;
            Word blue=root(w,destination(edge));if (red_number(w,blue)<sequence_cells) continue;
            if (promote==~0u) promote=blue;
            bool terminal=!vote(lane<mind.motors && w.edges[blue][lane]);
            for (Word k=0;k<w.reds;++k) {
                Word evidence=0;
                if (!fold(w,w.red[k],blue,mind.motors,lane,evidence,false)) continue;
                Word score=1+evidence;
                if ((terminal || evidence>=floor) && score>best) {best=score;winner=blue;model=k;}
            }
            if (++looked>=budget) {stop=true;break;}
        }
        if (promote==~0u) break;
        if (winner!=~0u) {Word ignored=0;fold(w,w.red[model],winner,mind.motors,lane,ignored,true);}
        else {
            if (w.reds>=limit) break;
            if (!lane) w.red[w.reds++]=static_cast<unsigned short>(promote);sync();
        }
        // Forced closure occasionally unifies two previously recruited cells.
        if (!lane) {
            Word n=0;
            for (Word j=0;j<w.reds;++j) {Word q=root(w,w.red[j]),k=0;while (k<n && w.red[k]!=q) ++k;if (k==n) w.red[n++]=static_cast<unsigned short>(q);}
            w.reds=n;
        }
        sync();
    }
    for (Word i=lane;i<sequence_cells*sequence_motors;i+=32) {
        Word q=i/sequence_motors,a=i%sequence_motors,edge=q<w.reds ? w.edges[root(w,w.red[q])][a] : 0;
        Word dest=edge ? red_number(w,root(w,destination(edge))) : sequence_cells;
        mind.map.edge[q][a]=dest<sequence_cells ? contact(dest,emission(edge)) : 0;
    }
    if (!lane) {
        mind.anchored=(t.senses[at(t,offset)]&128)!=0;mind.map.size=w.reds;Word current=red_number(w,root(w,pos));mind.active=current<sequence_cells ? 1ull<<current : 0;
        mind.age=0;++mind.edits;
    }
    sync();
}
}
