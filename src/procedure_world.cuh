#pragma once
#include "procedure_recur.cuh"
#include "procedure_rhythm.cuh"

namespace axion::life::apprenticeship {
using namespace procedure;
constexpr Word actions=6,verbs=6;
struct Form {Byte size=0,bytes[12]{};};
struct Convention {Form name[actions+verbs]{};Byte law[verbs][6]{},length[verbs]{},arity[verbs]{},motor[actions]{};Word backwards=0;};
AX_FN void dialect(Convention &v,Word seed,Word style) {
    v.backwards=style==2;
    if (!style) return;
    for (Word j=0;j<actions+verbs;++j) {
        Word k=mix(seed+j*0x45d9f3bu);auto &f=v.name[j];f.size=3;
        // One discriminating sound per marker and two shared-dialect phones.
        // The listener has no access to this assignment or its length.
        Byte one=static_cast<Byte>(97+((j+(seed%12))%12));
        f.bytes[v.backwards ? 2 : 0]=one;
        f.bytes[1]=static_cast<Byte>(109+k%13);f.bytes[v.backwards ? 0 : 2]=static_cast<Byte>(109+(k/13)%13);
    }
    for (Word a=0;a<actions;++a) v.motor[a]=static_cast<Byte>('0'+(a+(seed%actions))%actions);
    for (Word k=0;k<verbs;++k) {
        Word draw=mix(seed^k*0xbd739a53u);
        v.arity[k]=static_cast<Byte>(k<2 ? 1 : 2);
        Word n=v.arity[k]+1+((draw>>8)%2);v.length[k]=static_cast<Byte>(n);
        for (Word j=0;j<n;++j) v.law[k][j]=static_cast<Byte>(call0+(v.arity[k]==1 ? 0 : (draw>>j)&1));
        v.law[k][0]=call0;if (v.arity[k]==2) v.law[k][n-1]=call1;
        if (draw&1024) {auto first=v.law[k][0];v.law[k][0]=v.law[k][n-1];v.law[k][n-1]=first;}
        if (k==1) v.law[k][1]=static_cast<Byte>('0'+(draw%actions));
    }
}
template<int N> struct Train {
    Byte bytes[N];Word n=0;bool full=false;
    AX_FN void push(Byte c) {if (n<N) bytes[n++]=c;else full=true;}
    AX_FN void add(Slice x) {for (Word i=0;i<x.n;++i) push(x.p[i]);}
    AX_FN Slice span() const {return {bytes,n};}
};
template<int N> AX_FN void utter(Train<N> &stream,const Form &word) {stream.add({word.bytes,word.size});}
template<int N> AX_FN void compose(Train<N> &x,const Convention &v,Word op,Slice a,Slice b) {
    if (!v.backwards) utter(x,v.name[actions+op]);
    x.add(a);if (v.arity[op]==2) x.add(b);
    if (v.backwards) utter(x,v.name[actions+op]);
}
template<int N> AX_FN void perform(Train<N> &y,const Convention &v,Word op,Slice a,Slice b) {
    for (Word k=0;k<v.length[op];++k) {Byte f=v.law[op][k];if (f==call0) y.add(a);else if (f==call1) y.add(b);else y.push(f);}
}
AX_FN Count distort(Count original,Word &weather,Word flips) {
    for (Word k=0;k<flips;++k) {weather=mix(weather+0xa660d013u);original^=1ull<<(weather&63);}return original;
}
AX_FN void listen(Mind &m,Slice x,Slice y,Word &weather,Word noise) {
    Byte input[255],echo[255];if (x.n>255 || y.n>255) return;
    for (Word i=0;i<x.n;++i) {Word j=percept(m.ear,distort(signal(x.p[i]),weather,noise),m.genes,true);if (!j) return;input[i]=static_cast<Byte>(j);}
    for (Word i=0;i<y.n;++i) {Word j=percept(m.ear,distort(signal(y.p[i]),weather,noise),m.genes,true);if (!j) return;echo[i]=static_cast<Byte>(j);}
    retain(m.memory,{input,x.n},{echo,y.n});
}
AX_FN void listen_bursts(Mind &m,Slice x,Slice y,Word &weather,Word noise,Word rate) {
    Train<255> heard;Byte result[255];Hearing h;bool complete=true;
    auto emit=[&](Count wave) {Word id=percept(m.ear,wave,m.genes,true);heard.push(static_cast<Byte>(id));complete&=id!=0;};
    auto frame=[&](Count wave) {sample(h,wave,m.genes,emit);};
    for (Word i=0;i<x.n;++i) phonation(signal(x.p[i]),weather,rate,noise,false,frame);
    release(h,m.genes,emit);
    for (Word i=0;i<y.n;++i) {Word id=percept(m.ear,signal(y.p[i]),m.genes,true);complete&=id!=0;result[i]=static_cast<Byte>(id);}
    if (complete && !heard.full) retain(m.memory,heard.span(),{result,y.n});
}
template<class Port> AX_FN void show(const Convention &v,Word seed,Word repetitions,Port &port) {
    for (Word a=0;a<actions;++a) port(Slice{v.name[a].bytes,v.name[a].size},Slice{&v.motor[a],1});
    for (Word round=0;round<repetitions;++round) for (Word op=0;op<verbs;++op) {
        Word a=(round+op+seed)%actions,b=(a+1+(mix(seed+round*11+op)%5))%actions;
        Train<64> x,y;
        compose(x,v,op,{v.name[a].bytes,v.name[a].size},{v.name[b].bytes,v.name[b].size});
        perform(y,v,op,{&v.motor[a],1},{&v.motor[b],1});port(x.span(),y.span());
    }
}
struct RouteLaw {Form label;Byte go[actions]{},back[actions]{};};
AX_FN RouteLaw foray(const Convention &v,Word seed,Word style) {
    RouteLaw law;law.label.size=5;
    const Byte plain[]{'t','r','a','c','e'};
    for (Word i=0;i<5;++i) law.label.bytes[i]=style ? static_cast<Byte>('m'+mix(seed+(i+3)*147u)%13) : plain[i];
    for (Word i=0;i<actions;++i) {law.go[i]=v.motor[i];law.back[i]=static_cast<Byte>('A'+(i+(style ? seed%actions : 0))%actions);}
    return law;
}
template<int N> AX_FN void route_example(const Convention &v,const RouteLaw &law,const Byte *steps,Word n,Train<N> &x,Train<N> &y) {
    utter(x,law.label);
    for (Word k=0;k<n;++k) {utter(x,v.name[steps[k]]);y.push(law.go[steps[k]]);}
    for (Word k=n;k>0;--k) y.push(law.back[steps[k-1]]);
}
template<class Port> AX_FN void routes(const Convention &v,Word seed,Word style,bool sparse,Port &port) {
    auto law=foray(v,seed,style);
    for (Word i=0;i<1+actions+(sparse ? 2u : 2*actions);++i) {
        Byte moves[2]{};Word n=i==0 ? 0 : i<=actions ? 1 : 2;
        moves[0]=static_cast<Byte>(i<=actions ? (i ? i-1 : 0) : (i-1-actions)%actions);
        moves[1]=static_cast<Byte>((moves[0]+1+(i>2*actions))%actions);
        Train<64> x,y;route_example(v,law,moves,n,x,y);port(x.span(),y.span());
    }
}

}
