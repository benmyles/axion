#pragma once
#include "procedure_roost.cuh"

namespace axion::life::procedure {
enum Step : Word { finished,moved,waiting,broken };
struct Motion {Count emitted=0,tick=0;Word status=finished,depth=0;};
AX_FN Motion advance(const Mind &m,const Decoder &d,const Node *nodes,Performance &e,Word view,bool open) {
    Motion r;
    while (e.active && e.depth) {
        auto &pc=e.stack[e.depth-1];if (!pc.node || pc.node>e.nodes) {r.status=broken;break;}
        auto &n=nodes[pc.node];
        if (!pc.branch) {
            int branch=switch_to(m.circuit,n.rule,view,open);
            if (branch<0) {r.status=waiting;r.depth=e.depth;r.tick=e.moves;return r;}
            pc.branch=static_cast<Byte>(branch+1);
        }
        if (pc.branch==2 && !(m.circuit.cell[n.rule].flags>>2)) {r.status=broken;break;}
        auto ops=part_action(m.circuit,n.rule,pc.branch==2);
        if (pc.pc>=ops.n) {--e.depth;continue;}
        Byte op=ops.p[pc.pc];Word child=op==call0 ? n.left : n.right;
        if (op==call0 || op==call1) {
            if (e.depth==performance_depth || !child) {r.status=broken;break;}
            ++pc.pc;e.stack[e.depth++]={static_cast<Half>(child),0,0,0};continue;
        }
        if (var(op)) {
            auto s=bound({e.input,e.size},n,d.shape[n.rule],slot(op));
            if (pc.offset>=s.n) {pc.offset=0;++pc.pc;continue;}
            op=s.p[pc.offset++];
        } else ++pc.pc;
        if (e.moves==~0ull) {r.status=broken;break;}
        r.emitted=m.ear.model[op];r.status=moved;r.depth=e.depth;r.tick=++e.moves;return r;
    }
    e.active=0;r.tick=e.moves;return r;
}
AX_FN Word prepare(const Mind &m,const Decoder &d,Slice text,Tissue &t,Performance &e) {
    if (text.n>performance_span || (!d.direction && text.n>64)) return capacity;
    Report r;t.structure=true;
    Word root=read(m,d,t,text,r);if (!root) return r.status;
    for (Word i=0;i<text.n;++i) e.input[i]=text.p[i];
    e.size=static_cast<Half>(text.n);e.depth=1;e.stack[0]={static_cast<Half>(root),0,0,0};
    e.nodes=t.n;e.revision=m.revisions;e.active=1;e.moves=0;return recognized;
}
}
namespace axion::life::apprenticeship {
using namespace procedure;
struct WeatherGate {Form cue,middle;Word a=0,b=0,lut=6,reverse=0;};
AX_FN WeatherGate gate_law(Word key,Word style) {
    WeatherGate g;g.reverse=style==2;
    g.a=key&7;g.b=(g.a+1+((key>>8)%7))&7;
    const Word lut[]{2,4,6,7,8,9,11,13};g.lut=lut[(key>>21)&7];
    if (!style) {g.cue={5,{'w','a','t','c','h'}};g.middle={2,{'o','r'}};}
    else {g.cue={3,{'z',static_cast<Byte>('a'+((key>>8)%12)),'z'}};g.middle={2,{'z','y'}};}
    return g;
}
AX_FN bool opens(WeatherGate g,Word view) {return (g.lut>>(((view>>g.a)&1)|(((view>>g.b)&1)<<1)))&1;}
template<int N> AX_FN void contingent(Train<N> &x,const WeatherGate &g,const Form &one,const Form &two) {
    if (!g.reverse) utter(x,g.cue);
    utter(x,one);utter(x,g.middle);utter(x,two);if (g.reverse) utter(x,g.cue);
}
AX_FN void demonstrate_gate(Mind &m,const Convention &v,Word key,Word style,Word rounds,Word tempo,Word noise) {
    auto g=gate_law(key,style);Word weather=key;
    for (Word j=0;j<rounds;++j) {
        Word bits=mix(key^j*0x7f4a7c15u),a=(j+(key&31))%actions,b=(a+1+(bits%5))%actions;
        Word view=bits&255;view=(view&~((1u<<g.a)|(1u<<g.b)))|((j&1)<<g.a)|(((j>>1)&1)<<g.b);
        Train<64> x;contingent(x,g,v.name[a],v.name[b]);Byte action=v.motor[opens(g,view) ? b : a];
        if (tempo) listen_bursts(m,x.span(),{&action,1},weather,noise,tempo);
        else listen(m,x.span(),{&action,1},weather,noise);
        if (m.memory.count) {m.memory.visible[m.memory.count-1]=1;m.memory.retina[m.memory.count-1]=static_cast<Byte>(view);}
    }
}
struct PerformanceCase {Node nodes[48]{};Performance start{};Word valid=0;Byte stimulus[5]{};};
AX_FN PerformanceCase concert(const Mind &m,const Decoder &decoder,const Convention &v,Word key,Word style) {
    auto g=gate_law(key,style);PerformanceCase c;Train<96> guard,repeated,command;
    // The first motion precedes the guarded subtree. Two invocations of the
    // same subtree independently hear the physical scene at the time of use.
    contingent(guard,g,v.name[1],v.name[2]);
    Word double_op=0,join_op=actions;
    for (Word j=0;j<verbs;++j) if (v.arity[j]==2 && v.length[j]>=2) {join_op=j;break;}
    compose(repeated,v,double_op,guard.span(),{});compose(command,v,join_op,{v.name[0].bytes,v.name[0].size},repeated.span());
    for (Word k=0;k<command.n;++k) {Word id=roost::detect(m.ear,signal(command.bytes[k]),m.genes);if (!id) return c;command.bytes[k]=static_cast<Byte>(id);}
    Summary sums[48];Half grid[4096];procedure::Tissue t{c.nodes,sums,grid,47,0,true};
    c.valid=prepare(m,decoder,command.span(),t,c.start)==recognized;
    return c;
}
}
