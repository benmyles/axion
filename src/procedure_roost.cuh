#pragma once
#include "procedure_gait.cuh"
#include "procedure_express.cuh"

namespace axion::life::roost {
using namespace procedure;
constexpr Word words=72,movements=24;
struct Message {Count voice[words]{},deed[movements]{};Word n=0,steps=0,lesson=0;};
struct Day {
    Word energy=0,sorties=0,landings=0,meals=0,moves=0,utterances=0,spikes=0,receptions=0,understood=0;
    Word innovations=0,imitations=0,abbreviated=0,charged=0,changes=0,matched=0,reactive=0;
    Word site=0,replays=0,rewired=0,adopted=0,declined=0,success[3]{},attempts[3]{};
    // Sampled motor telemetry for the first island, outside the nervous system.
    Byte path[movements+1]{},path_n=0,fruit=64;Word path_tick=0,productions=0,receptors=0;Count call=0;
};
struct Terrain {Count buds=0;Word seed=0,visits=0;};
struct Flight {Count echo[movements]{};Byte path[movements]{};Word steps=0,end=0,patch=64,matched=0;};
AX_FN Word detect(const Ear &ear,Count pulse,RecollectionGenes genes) {
    Word chosen=0,best=65,other=65;
    for (Word j=1;j<=ear.size;++j) {
        Word d=__builtin_popcountll(ear.model[j]^pulse);
        if (d<best) {chosen=j;other=best;best=d;if (!d) break;}else if (d<other) other=d;
    }
    return best<other && best<=(genes.low&15) ? chosen : 0;
}
AX_FN Word narrate(const Mind &m,const Count *actual,Word steps,Count *vocal,Word cap) {
    Byte heard[movements],answer[words];if (!steps || steps>movements) return 0;
    for (Word i=0;i<steps;++i) {Word id=detect(m.ear,actual[i],m.genes);if (!id) return 0;heard[i]=static_cast<Byte>(id);}
    Slice stream{heard,steps};Word best=cap+1;
    for (Word i=0;i<m.recursion.n;++i) {
        Byte text[words];Word len=0;
        if (unwind(m.recursion.state[i],stream,text,cap,len) && len<best) {best=len;for (Word k=0;k<len;++k) answer[k]=text[k];}
    }
    for (Word i=0;i<m.circuit.count;++i) {
        auto r=m.circuit.cell[i];if (r.arity || r.input>=best || !same(output(m.circuit,i),stream)) continue;
        best=r.input;auto x=cue(m.circuit,i);for (Word k=0;k<x.n;++k) answer[k]=x.p[k];
    }
    if (best>cap) return 0;
    for (Word i=0;i<best;++i) vocal[i]=m.ear.model[answer[i]];
    return best;
}
AX_FN Word interpret(const Mind &m,const Count *input,Word n,Count *motor,Word cap,bool residual) {
    if (!n || n>words) return 0;
    Byte ids[words];
    for (Word k=0;k<n;++k) {Word id=detect(m.ear,input[k],m.genes);if (!id) return 0;ids[k]=static_cast<Byte>(id);}
    Slice x{ids,n};
    for (Word i=0;i<m.circuit.count;++i) if (!m.circuit.cell[i].arity && same(cue(m.circuit,i),x)) {
        auto plan=output(m.circuit,i);if (plan.n>cap) return 0;for (Word j=0;j<plan.n;++j) motor[j]=m.ear.model[plan.p[j]];return plan.n;
    }
    if (residual) for (Word i=0;i<m.recursion.n;++i) if (domain(m.recursion.state[i],x)) {
        auto out=recurrence(m.ear,m.recursion.state[i],x,0,cap,motor);
        return out.status==recognized && out.voice.length<=cap ? out.spoken : 0;
    }
    return 0;
}
AX_FN Word intended(const Mind &m,Word key,Count *goal,Word cap) {
    if (!m.recursion.n) return 0;
    const auto &p=m.recursion.state[key%m.recursion.n];Byte text[words];Word size=0;
    for (Word j=0;j<p.head;++j) text[size++]=p.prefix[j];
    Word n=2+(key>>12)%5;
    for (Word j=0;j<n;++j) {auto term=token(p,mix(key+j*19)%p.n);if (size+term.n+p.tail>words) return 0;
        for (Word k=0;k<term.n;++k) text[size++]=term.p[k];}
    for (Word j=0;j<p.tail;++j) text[size++]=p.suffix[j];
    auto actual=recurrence(m.ear,p,{text,size},0,cap,goal);
    return actual.status==recognized && actual.voice.length<=cap ? actual.spoken : 0;
}
AX_FN Flight fly(Mind &m,Genome g,const apprenticeship::RouteLaw &law,Wind w,Count buds,Word home,Word land,Word key,
                 const Count *desired,Word n,Word cut,Day &record,Kinetics *book=nullptr) {
    Flight f;f.end=home;Word first=mix(key+33)&255;
    if (n>movements) n=movements;
    for (Word k=0;k<n;++k) {
        Word noise=mix(key+k*0x7f4a7c15u+f.end),view=k ? noise&255 : first;
        Word pulse=muscle(m.muscle,desired[k],cut==3 ? first : view,noise);
        Word real=effect(w,view,pulse);Count felt=proprioception(law,real);
        bool commit=false;
        if (book) {
            Word id=hear(m.muscle,felt,true);
            if (id<effectors) {
                experience(*book,{static_cast<Byte>(view),static_cast<Byte>(pulse),static_cast<Byte>(id)},felt!=desired[k]);
                if (book->remaining) {
                    Word phase=weather(view,book->a,book->b),confidence=0,forecast=provisional(*book,phase,id);
                    book->challenger+=drive(forecast,noise,confidence)==pulse;
                    book->incumbent+=drive(m.muscle.contact[weather(view,m.muscle.first,m.muscle.second)][id],noise,confidence)==pulse;
                    provisional(*book,phase,id,plastic(forecast,pulse,g.learning.low>>16));
                    if (!--book->remaining) {
                        commit=book->challenger>book->incumbent+((g.fecundity>>24)&3);
                        if (commit) ++record.adopted;else ++record.declined;
                    }
                }
            }
        }
        if (cut!=4) feel(m.muscle,g.learning,view,pulse,felt);
        if (commit) {
            record.rewired+=m.muscle.first!=book->a || m.muscle.second!=book->b;
            m.muscle.first=book->a;m.muscle.second=book->b;
            for (Word j=0;j<4;++j) for (Word e=0;e<effectors;++e) m.muscle.contact[j][e]=provisional(*book,j,e);
        }
        f.end=move(f.end,real,land);f.path[f.steps]=static_cast<Byte>(f.end);f.echo[f.steps++]=felt;record.reactive+=felt==desired[k];
        if (f.patch==64 && f.end!=home && (buds&(1ull<<f.end))) f.patch=f.end;
    }
    ++record.sorties;record.moves+=f.steps;record.landings+=f.end==home;return f;
}

#if defined(__CUDA__)
AX_FN Word elect(Word address,Word lane,Word seed) {
    Word same;asm volatile("match.any.sync.b32 %0,%1,0xffffffff;":"=r"(same):"r"(address));
    Word rotation=seed&31,order=(same>>rotation)|(same<<((32-rotation)&31));
    return address<64 && lane==((Word(__builtin_ctz(order))+rotation)&31);
}
AX_FN Count gathered(Count take) {
    Word a=Word(take),b=Word(take>>32);
    for (Word shift=16;shift;shift>>=1) {Word x,y;
        asm volatile("shfl.sync.bfly.b32 %0,%1,%2,31,0xffffffff;":"=r"(x):"r"(a),"r"(shift));
        asm volatile("shfl.sync.bfly.b32 %0,%1,%2,31,0xffffffff;":"=r"(y):"r"(b),"r"(shift));a|=x;b|=y;
    }
    return Count(a)|(Count(b)<<32);
}

#endif
AX_FN Count season(Count old,Word key) {
    Count west=((old&0x7f7f7f7f7f7f7f7full)<<1)|((old&0x8080808080808080ull)>>7);
    Count north=(old<<8)|(old>>56);Count rain=Count(mix(key))|(Count(mix(key+173))<<32);
    Count spores=rain&(Count(mix(key+31))|(Count(mix(key+771))<<32));
    return (old|(west&north&rain)|(spores&(spores>>5)))&~1ull;
}
AX_FN Word named(Mind &m,const Count *label,Word n,const Count *movement,Word steps,bool coined) {
    if (!n || !steps || n>motif || steps>motif) return 0;
    Draft d;d.l=n;d.r=steps;
    for (Word j=0;j<n;++j) {Word id=percept(m.ear,label[j],m.genes,true);if (!id) return 0;d.in[j]=static_cast<Byte>(id);}
    for (Word j=0;j<steps;++j) {Word id=percept(m.ear,movement[j],m.genes,true);if (!id) return 0;d.out[j]=static_cast<Byte>(id);}
    for (Word j=0;j<m.circuit.count;++j) if (same({d.in,d.l},cue(m.circuit,j))) return 0;
    if (!affordable(m,{d.in,d.l}) || !append(m.circuit,d,1)) return 0;
    m.circuit.cell[m.circuit.count-1].flags=coined ? 1 : 2;retain(m.memory,{d.in,d.l},{d.out,d.r});m.performance.active=0;++m.revisions;return 1;
}
AX_FN Word coin(Mind &m,Word key,const Flight &f,Count *word) {
    if (!(m.genes.high&16) || !m.memory.count || !f.steps || m.circuit.count>=productions-1) return 0;
    for (Word k=0;k<64;++k) {
        Word size=1+(key&1);Byte sound[2];
        for (Word j=0;j<size;++j) {
            Word sample=mix(key+k*53+j*961)%m.memory.count;auto heard=before(m.memory,sample);
            sound[j]=heard.p[mix(key+sample+k)%heard.n];word[j]=m.ear.model[sound[j]];
        }
        if (affordable(m,{sound,size}) && named(m,word,size,f.echo,f.steps,true)) return size;
    }
    return 0;
}
AX_FN Word hearing(const Mind &adult,const Decoder &d,Slice text,Count *wave,Word cap,procedure::Tissue &t) {
    for (Word k=0;k<adult.recursion.n;++k) if (domain(adult.recursion.state[k],text)) {
        auto v=recurrence(adult.ear,adult.recursion.state[k],text,0,cap,wave);return v.status==recognized && v.voice.length<=cap ? v.spoken : ~0u;
    }
    if (!d.direction && text.n>64) return ~0u;
    Report r;Word root=read(adult,d,t,text,r);if (!root || r.status!=recognized || r.voice.length>cap) return ~0u;
    for (Word j=0;j<r.voice.length;++j) wave[j]=adult.ear.model[sound_at(adult,d,t,text,root,j)];
    return Word(r.voice.length);
}
AX_FN void rear_from(Mind &child,const Mind &adult,Word seed,Word cut) {
    Decoder d;compile(adult.circuit,d);Node nodes[129];Summary summary[129];Half chart[4096];
    procedure::Tissue tissue{nodes,summary,chart,128,0};
    for (Word k=0;k<adult.memory.count;++k) {
        Word idx=(k+(seed%adult.memory.count))%adult.memory.count;auto x=before(adult.memory,idx);Count waves[128];Byte heard[255],reply[128];
        tissue.override_eyes=true;tissue.retina=adult.memory.retina[idx];tissue.seeing=adult.memory.visible[idx];
        Word n=hearing(adult,d,x,waves,128,tissue);
        if (n==~0u || cut==1) continue;
        bool ok=true;
        for (Word j=0;j<x.n;++j) {
            Word from=cut==2 ? x.n-1-j : j,id=percept(child.ear,adult.ear.model[x.p[from]],child.genes,true);
            heard[j]=static_cast<Byte>(id);ok&=id!=0;
        }
        for (Word j=0;j<n;++j) {Word id=percept(child.ear,waves[j],child.genes,true);reply[j]=static_cast<Byte>(id);ok&=id!=0;}
        if (ok) retain(child.memory,{heard,x.n},{reply,n},tissue.retina,tissue.seeing!=0);
    }
}
}
