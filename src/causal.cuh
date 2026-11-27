#pragma once
#include "axion/causal.hpp"

namespace axion::life::causal {
// Each half-word indexes all unordered pairs of the six receptors.
struct Receptors {
    Count left=0,right=0;
    unsigned short first[traits]{},second[traits]{};
    constexpr Receptors() {
        Word j=0;
        for (Word a=0;a<traits;++a) for (Word b=a+1;b<traits;++b,++j) {
            left|=Count(a)<<(4*j);right|=Count(b)<<(4*j);
            first[a]|=1u<<j;second[b]|=1u<<j;
        }
    }
};
constexpr Receptors receptors{};
AX_FN Word from(Word pair) {return Word(receptors.left>>(4*pair))&7;}
AX_FN Word other(Word pair) {return Word(receptors.right>>(4*pair))&7;}
struct Patches {Word group[4];};
AX_FN Patches patterns(Word state) {
    // Unrolling keeps the wiring addresses immediate in PTX.
    Word a=0,b=0;
#if defined(__CUDA__)
#pragma unroll
#endif
    for (Word i=0;i<traits;++i) if (state&(1u<<i)) {a|=receptors.first[i];b|=receptors.second[i];}
    return {{~(a|b)&all_pairs,a&~b,b&~a,a&b}};
}
AX_FN Word table(Word bits,Word a,Word b) {
    Word p=a&b,q=a&~b,r=b&~a,s=~(a|b);
    return ((bits&1 ? s : 0)|(bits&2 ? q : 0)|(bits&4 ? r : 0)|(bits&8 ? p : 0))&all_pairs;
}
AX_FN Word gate(Word a,Word b,Word truth) {return 1+a+(b<<3)+(truth<<6);}
AX_FN Word canonical(Word a,Word b,Word v) {
    if (v==0 || v==15) return gate(0,0,v);
    if ((v&3)==(v>>2)) return gate(a,a,(v&1) ? 5 : 10);
    if ((v&5)==((v>>1)&5)) return gate(b,b,(v&1) ? 5 : 10);
    return gate(a,b,v);
}
AX_FN Word evaluate(Word code,Word state) {
    Word c=code-1,i=(state>>(c&7))&1,j=(state>>((c>>3)&7))&1;
    return (c>>(6+i+2*j))&1;
}
struct Signals {Word off=0,on=0;};
AX_FN Signals express(RecollectionGenes g,Word a,Word b) {
    return {table(g.high&15,a,b),table(g.high&15,b,a)};
}
AX_FN Word viable(const CausalField &f,RecollectionGenes g) {
    Word rejected=0,veto=(g.high>>4)&15;
    for (Word k=0;k<4;++k) rejected|=table(veto,f.low[k],f.high[k]);
    return all_pairs&~rejected;
}
AX_FN void consolidate(CausalField &f,RecollectionGenes g) {
    Word live=viable(f,g);f.viable=static_cast<unsigned short>(live);f.compiled=0;
    if (!live) return;
    Word ones[4];
    for (Word k=0;k<4;++k) {
        auto s=express(g,f.low[k],f.high[k]);if ((live&(s.off^s.on))!=live) return;ones[k]=s.on;
    }
    Word common=0;
    while (live) {
        Word j=Word(__builtin_ctz(live));live&=live-1;
        Word v=0;for (Word k=0;k<4;++k) v|=((ones[k]>>j)&1)<<k;
        Word code=canonical(from(j),other(j),v);if (common && code!=common) return;common=code;
    }
    f.compiled=static_cast<unsigned short>(common);
}
AX_FN void write(CausalField &f,RecollectionGenes g,Patches p,Word available,Word output,bool changed) {
    Word offset=8*Word(changed);
    for (Word k=0;k<4;++k) {
        Word active=p.group[k]&available,a=f.low[k],b=f.high[k];
        Word lo=table((g.low>>(offset+4*!output))&15,a,b);
        Word hi=table((g.low>>(offset+4*output))&15,b,a);
        f.low[k]=static_cast<unsigned short>((a&~active)|(lo&active));
        f.high[k]=static_cast<unsigned short>((b&~active)|(hi&active));
    }
}
AX_FN void learn_field(CausalField &f,RecollectionGenes g,Patches p,Word available,Word output,bool changed) {
    bool used=f.viable!=0;write(f,g,p,available,output,changed);consolidate(f,g);
    if (!f.viable && used && (g.high&(1u<<8))) {
        f={};write(f,g,p,available,output,changed);consolidate(f,g);
    }
}
AX_FN void learn(CausalModel &m,Intervention e) {
    if (e.action>=operations || e.before>all_traits || e.after>all_traits) return;
    Patches p=patterns(e.before);Word available=patterns(e.known_before).group[3];
    if (!available) return;
    if (e.known_before==all_traits && e.known_after==all_traits) m.visited[e.action]|=1ull<<e.before;
    for (Word i=0;i<traits;++i) if (e.known_after&(1u<<i)) {
        bool changed=((e.before^e.after)&e.known_before&(1u<<i))!=0;
        learn_field(m.fields[e.action][i],m.genes,p,available,(e.after>>i)&1,changed);
    }
}
AX_FN Word read_field(const CausalField &f,RecollectionGenes g,Word state,Patches p) {
    if (f.compiled) return 1u<<evaluate(f.compiled,state);
    if (!f.viable) return 3;
    Word lo=0,hi=0;
    for (Word k=0;k<4;++k) {auto s=express(g,f.low[k],f.high[k]);Word fired=f.viable&p.group[k];lo|=s.off&fired;hi|=s.on&fired;}
    Word result=Word(lo!=0)+2*Word(hi!=0);return result ? result : 3;
}
AX_FN CausalStep predict(const CausalModel &m,Word action,Word state,bool episodic=false) {
    if (action>=operations || (episodic && !(m.visited[action]&(1ull<<state)))) return {0,all_traits};
    CausalStep out{};auto p=patterns(state);
    for (Word j=0;j<traits;++j) {
        Word result=read_field(m.fields[action][j],m.genes,state,p);
        if (result==2) out.on|=1u<<j;
        if (result==3) out.unknown|=1u<<j;
    }
    return out;
}
AX_FN Count plane(Word i) {
    constexpr Count bits[traits]{0xaaaaaaaaaaaaaaaaull,0xccccccccccccccccull,0xf0f0f0f0f0f0f0f0ull,
                                0xff00ff00ff00ff00ull,0xffff0000ffff0000ull,0xffffffff00000000ull};
    return bits[i];
}
AX_FN Count matches(Word require,Word value) {
    Count states=~0ull;
#if defined(__CUDA__)
#pragma unroll
#endif
    for (Word j=0;j<traits;++j) if (require&(1u<<j)) states&=(value&(1u<<j)) ? plane(j) : ~plane(j);
    return states;
}
AX_FN Count successors(CausalStep p) {return p.unknown ? matches(all_traits&~p.unknown,p.on) : 1ull<<p.on;}
AX_FN bool recruit_goal(RecollectionGenes g,Count next,Count active) {
    Word rule=(g.expression>>16)&3;Count excited=next&active,quiet=next&~active;
    if (rule==0) return !quiet;
    if (rule==1) return excited!=0;
    if (rule==2) return __builtin_popcountll(excited)>__builtin_popcountll(quiet);
    return !quiet && !(next&(next-1));
}
AX_FN Word physical(const Apparatus &world,Word action,Word state) {
    Word next=0;for (Word j=0;j<traits;++j) next|=evaluate(world.gates[action][j],state)<<j;return next;
}
}
