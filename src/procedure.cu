#include "procedure_feedback.cuh"
using namespace axion;
using namespace axion::life;
using namespace axion::life::procedure;
#define ENTRY extern "C" __attribute__((global))
AX_FN Word lane() {Word i;asm("mov.u32 %0, %%tid.x;":"=r"(i));return i;}
AX_FN Word group() {Word i;asm("mov.u32 %0, %%ctaid.x;":"=r"(i));return i;}
ENTRY void procedure_rear(Mind *m,const apprenticeship::Convention *base,const RecollectionGenes *genes,Word siblings,Word n,
                          Word seed,Word style,Word rounds,Word noise,Word journeys,Word tempo,Word guards) {
    Word i=group()*32+lane();if (i>=n) return;
    auto &s=m[i];s={};s.genes=genes[i/siblings];Word key=mix(seed+(i%siblings)*0x9e3779b9u),weather=mix(key+35);auto v=*base;
    apprenticeship::dialect(v,key,style);
    auto hear=[&](Slice x,Slice y) {if (tempo) apprenticeship::listen_bursts(s,x,y,weather,noise,tempo);else apprenticeship::listen(s,x,y,weather,noise);};
    apprenticeship::show(v,key,rounds,hear);if (journeys) apprenticeship::routes(v,key,style,journeys==2,hear);
    if (guards) apprenticeship::demonstrate_gate(s,v,key,style,guards,tempo,noise);
}
ENTRY void procedure_trace(Mind *m,const Count *pulses,Word n,Word y,Word view,Word seeing,Word *update) {
    if (update) *update=0;
    if (n>255 || y>255 || !n) return;
    Byte input[255],reply[255];
    for (Word k=0;k<n+y;++k) {
        Word id=percept(m->ear,pulses[k],m->genes,true);if (!id) return;
        if (k<n) input[k]=static_cast<Byte>(id);else reply[k-n]=static_cast<Byte>(id);
    }
    Word stimulus=1;
    if (update) for (Word k=m->memory.count;k--;) {
        if (!same(before(m->memory,k),{input,n})) continue;
        if (m->memory.visible[k]!=seeing || (seeing && m->memory.retina[k]!=view)) {stimulus=2;continue;}
        stimulus=same(after(m->memory,k),{reply,y}) ? 0 : 3;break;
    }
    bool stored=observe(*m,{input,n},{reply,y},view,seeing);
    if (update) *update=Word(stored && ((m->genes.expression>>(27+stimulus))&1));
}
ENTRY void procedure_recursion(Mind *m,Fit *fit,Word n) {
    Word i=group()*32+lane();if (i>=n) return;
    auto &s=m[i];auto &f=fit[i];f.clipped=0;for (Word j=0;j<s.memory.count;++j) f.folded[j]=0;
    f.acquired=s.recursion;if (!(s.genes.high&8)) return;
    auto &next=f.acquired;
    for (Word j=0;j<s.memory.count;++j) {
        auto anchor=before(s.memory,j);if (s.memory.visible[j] || anchor.n>motif || f.folded[j]) continue;
        Induction best;Word rank=0;
        for (Word k=0;k<=anchor.n;++k) {
            Induction trial;if (!induce_loop(s,j,k,trial)) continue;
            Word cost=trial.units+trial.unresolved;Word score=trial.observations*64+(cost<63 ? 63-cost : 0);
            if (score>rank) {best=trial;rank=score;}
        }
        if (!rank) continue;
        if (!save_loop(next,best)) {f.clipped=1;return;}
        for (Word q=0;q<s.memory.count;++q) {
            auto x=before(s.memory,q);
            if (addressed(x,{best.prefix,best.head},{best.suffix,best.tail}) && (x.n==best.head+best.tail || scan_units(best,part(x,best.head,x.n-best.head-best.tail),nullptr)))
                f.folded[q]=1;
        }
    }
}
ENTRY void procedure_sites(const Mind *m,Fit *scratch) {
    Word i=group(),row=lane();if (row<m[i].memory.count) boundaries(m[i],row,scratch[i].origin[row]);
}
ENTRY void procedure_align(const Mind *m,Fit *scratch) {
    Word i=group(),row=lane();if (row>=m[i].memory.count) return;
    auto &s=scratch[i];s.guarded[row].length=0;
    if (m[i].memory.visible[row]) {
        OutcomeGrammar candidate;guard_candidate(m[i],s.origin[row],row,candidate);s.quality[row]={candidate.observations,candidate.score};
        if (candidate.score) {s.proposed[row]=candidate.rule;auto &g=s.guarded[row];g.gate=candidate.field;g.length=candidate.alternative;
            for (Word k=0;k<g.length;++k) g.alternative[k]=candidate.alt[k];}return;
    }
    if (s.folded[row]) {s.quality[row]={};return;}proposal(m[i],s.origin[row],row,s.proposed[row],s.quality[row]);
}
AX_FN Draft reconstruct(const Circuit &p,Word j) {
    auto r=p.cell[j];auto x=cue(p,j),y=output(p,j);Draft d;d.l=x.n;d.r=y.n;d.raw=r.raw;d.arity=r.arity;
    for (Word k=0;k<x.n;++k) d.in[k]=x.p[k];
    for (Word k=0;k<y.n;++k) d.out[k]=y.p[k];return d;
}
ENTRY void procedure_connect(Mind *m,Fit *scratch,Word *clipped,Word n) {
    Word i=group()*32+lane();if (i>=n) return;
    auto &s=m[i];auto &book=s.memory;auto &w=scratch[i];Circuit next;clipped[i]=1;if (w.clipped) return;
    for (Word r=0;r<s.circuit.count;++r) {
        auto old=reconstruct(s.circuit,r);bool superseded=false;Word flags=s.circuit.cell[r].flags;
        if (s.genes.expression&(1u<<30)) for (Word j=0;j<book.count && !superseded;++j) {
            if (!old.arity && !book.visible[j] && same(before(book,j),{old.in,old.l}))
                superseded=!same(after(book,j),{old.out,old.r});
            if (old.arity && w.quality[j].score) {
                const auto &fresh=w.proposed[j];
                if (fresh.raw==old.raw && same({fresh.in,fresh.l},{old.in,old.l}))
                    superseded=(flags>>2) || !same({fresh.out,fresh.r},{old.out,old.r});
            }
        }
        if (superseded) continue;
        append(next,old,s.circuit.cell[r].support,true);
        if (flags>>2) {next.guards[next.guard_n++]=s.circuit.guards[(flags>>2)-1];flags=(flags&3)|(next.guard_n<<2);}
        next.cell[next.count-1].flags=static_cast<Byte>(flags);
    }
    for (Word j=0;j<book.count;++j) if (w.quality[j].score) {
        auto &d=w.proposed[j];
        if (!book.visible[j]) {if (!append(next,d,w.quality[j].support)) {w.clipped=1;return;}continue;}
        Word existing=0;while (existing<next.count && !same(cue(next,existing),{d.in,d.l})) ++existing;
        auto &candidate=w.guarded[j];if (existing<next.count) continue;
        if (next.guard_n==guard_limit || !append(next,d,w.quality[j].support,true)) {w.clipped=1;return;}
        auto &gate=next.guards[next.guard_n++];gate=candidate.gate;gate.length=static_cast<Byte>(candidate.length);
        for (Word k=0;k<gate.length;++k) gate.effect[k]=candidate.alternative[k];
        next.cell[next.count-1].flags=static_cast<Byte>(next.guard_n<<2);
    }
    for (Word row=0;row<book.count;++row) {
        if (w.folded[row] || book.visible[row]) continue;
        auto x=before(book,row),y=after(book,row);bool represented=false;
        for (Word j=0;j<next.count && !represented;++j) {
            auto c=next.cell[j];
            if (c.arity) represented=evidence(s,reconstruct(next,j),int(row)).support!=0;
            else represented=same(x,cue(next,j)) && same(y,output(next,j));
        }
        if (represented) continue;
        if (x.n>motif || y.n>motif) {w.clipped=1;return;}
        Draft literal;literal.l=x.n;literal.r=y.n;
        for (Word k=0;k<x.n;++k) literal.in[k]=x.p[k];for (Word k=0;k<y.n;++k) literal.out[k]=y.p[k];
        if (!append(next,literal,1)) {w.clipped=1;return;}
    }
    s.circuit=next;s.recursion=w.acquired;s.performance.active=0;++s.revisions;s.consolidated=book.observed;clipped[i]=0;
}
ENTRY void procedure_compile(const Mind *m,Decoder *d,Word n) {Word i=group()*32+lane();if (i<n) compile(m[i].circuit,d[i]);}
ENTRY void procedure_query(Mind *m,const Decoder *d,const Count *stimulus,Word length,Byte *ids,Node *nodes,Summary *plans,Half *grid,
                          Word limit,Report *out,Count *reply,Count from,Word requested,Word force_chart) {
    *out={};Word count=m->ear.size;bool accepts_raw=false;
    for (Word i=0;i<m->circuit.count;++i) accepts_raw|=m->circuit.cell[i].raw!=0;
    for (Word j=0;j<length;++j) {
        Word p=percept(m->ear,stimulus[j],m->genes,false);
        if (!p && accepts_raw && m->ear.size<letters && stimulus[j]) {
            p=++m->ear.size;m->ear.model[p]=stimulus[j];m->ear.strength[p]=0;
        }
        if (!p) {m->ear.size=count;for (auto &v:m->ear.cache) if (v>count) v=0;return;}ids[j]=static_cast<Byte>(p);
    }
    Slice x{ids,length};
    for (Word i=0;i<m->recursion.n;++i) if (domain(m->recursion.state[i],x)) {
        *out=recurrence(m->ear,m->recursion.state[i],x,from,requested,reply);return;
    }
    procedure::Tissue tissue{nodes,plans,grid,limit,0};Decoder decoder=*d;if (force_chart) decoder.direction=0;
    if (!decoder.direction && !grid) {out->status=capacity;m->ear.size=count;for (auto &v:m->ear.cache) if (v>count) v=0;return;}
    Word root=read(*m,decoder,tissue,x,*out);
    if (out->status==needs_observation) return;
    if (!root) {m->ear.size=count;for (auto &v:m->ear.cache) if (v>count) v=0;return;}
    for (Word j=0;j<requested && from<out->voice.length;++j,++from) {
        reply[j]=m->ear.model[sound_at(*m,decoder,tissue,x,root,from)];++out->spoken;
    }
}

ENTRY void procedure_ask(const Mind *m,Inquiry *q,Word seed,Word random) {*q=question(*m,seed,random!=0);}
ENTRY void procedure_tuition(Mind *m,const apprenticeship::Convention *base,Word n,Word seed,Word style,Word budget,Word random,Word *counts) {
    Word i=group()*32+lane();if (i>=n) return;
    auto &s=m[i];Word key=mix(seed+i*0x9e3779b9u),weather=key;auto d=*base;apprenticeship::dialect(d,key,style);
    auto law=apprenticeship::foray(d,key,style);Word asked=0,paid=0;
    if (random==2) budget=counts[i];
    for (;asked<budget;++asked) {
        auto q=question(s,mix(key+asked),random!=0,random==2);if (!q.length) break;
        // An independent caregiver hears the proposed waves. It reads its
        // own acoustic motor repertoire, without seeing the pupil's cuts.
        Byte raw[96],moves[32];Word nr=0,pos=law.label.size;bool understood=true;
        for (Word j=0;j<q.length;++j) {
            Count pulse=s.ear.model[q.signal[j]];Word ch=1;while (ch<128 && signal(ch)!=pulse) ++ch;
            if (ch==128) {understood=false;break;}raw[j]=static_cast<Byte>(ch);
        }
        if (!understood || !at({raw,q.length},0,{law.label.bytes,law.label.size})) break;
        while (pos<q.length && nr<32) {
            Word a=0;while (a<apprenticeship::actions && !at({raw,q.length},pos,{d.name[a].bytes,d.name[a].size})) ++a;
            if (a==apprenticeship::actions) break;moves[nr++]=static_cast<Byte>(a);pos+=d.name[a].size;
        }
        if (pos!=q.length) break;
        apprenticeship::Train<96> input,output;apprenticeship::route_example(d,law,moves,nr,input,output);
        apprenticeship::listen(s,input.span(),output.span(),weather,0);++s.conversations;++paid;
        auto &r=s.recursion.state[q.scope];Word row=0;
        for (;row<s.memory.count;++row) {auto x=before(s.memory,row);if (x.n==r.head+r.tail && matching(r,x)) break;}
        Induction learned;if (row==s.memory.count || !induce_loop(s,row,r.head,learned)) break;
        save_loop(s.recursion,learned);
    }
    counts[i]=paid;
}
#include "procedure_probe.cuh"
ENTRY void procedure_probes(Mind *m,const Decoder *d,const apprenticeship::Convention *base,apprenticeship::Probes *score,
            Node *nodes,Summary *plans,Half *chart,Word n,Word first,Word siblings,Word seed,Word style,Word probes,Word depth,Word noise,Word limit) {
    Word local=group()*32+lane();if (local>=n) return;Word i=first+local,key=mix(seed+(i%siblings)*0x9e3779b9u);
    auto v=*base;apprenticeship::dialect(v,key,style);
    procedure::Tissue tissue{nodes+Count(local)*(limit+1),plans+Count(local)*(limit+1),chart+Count(local)*64*64,limit,0};
    apprenticeship::examine(m[i],d[i],v,apprenticeship::foray(v,key,style),mix(key+0x67bb5a13u),probes,depth,noise,tissue,score[i]);
}
ENTRY void procedure_merit(const apprenticeship::Probes *scores,Word *fitness,Word n,Word siblings) {
    Word i=group()*32+lane();if (i>=n) return;Word sum=0;
    for (Word j=0;j<siblings;++j) {
        const auto &s=scores[i*siblings+j];sum+=12*s.perfect+10*s.returned+3*s.answered+((s.samples_ok*32)/(s.samples ? s.samples : 1));
    }fitness[i]=sum;
}
#include "procedure_express.cuh"
ENTRY void procedure_express(Mind *m,const Count *waves,Word n,Byte *target,Derivation *chart,Byte *ids,Count *text,Word max,Description *result) {
    Word t=lane();
    if (!t) *result={};
    for (Word k=t;k<n;k+=128) {
        // Read-only matching avoids racing the mutable receptor cache.
        Word cell=0,distance=65,next=65;
        for (Word j=1;j<=m->ear.size;++j) {
            Word d=__builtin_popcountll(m->ear.model[j]^waves[k]);
            if (d<distance) {cell=j;next=distance;distance=d;}else if (d<next) next=d;
        }
        target[k]=distance<next && distance<=(m->genes.low&15) ? static_cast<Byte>(cell) : 0;
    }
    asm volatile("bar.sync 0;":::"memory");
    for (Word length=1;length<=n;++length) {
        for (Word lo=t;lo+length<=n;lo+=128) parse_at(chart,n,lo,length)=infer_inverse(*m,{target,n},lo,length,chart);
        asm volatile("bar.sync 0;":::"memory");
    }
    if (!t) *result=articulate(*m,{target,n},chart,ids,max);
    asm volatile("bar.sync 0;":::"memory");
    if (result->status==recognized) for (Word k=t;k<result->length;k+=128) text[k]=m->ear.model[ids[k]];
}
ENTRY void procedure_innovate(Mind *m,Invention *out,Word n,Word seed,Word maximum) {
    Word i=group()*32+lane();if (i>=n) return;
    auto &s=m[i];auto proposal=innovate(s,mix(seed+i),maximum);
    if (proposal.length) {
        Draft d;d.l=proposal.length;d.r=proposal.actions;
        for (Word j=0;j<d.l;++j) d.in[j]=proposal.sign[j];for (Word j=0;j<d.r;++j) d.out[j]=proposal.movement[j];
        Word prior=s.circuit.count;
        if (append(s.circuit,d,proposal.exposures)) {
            if (s.circuit.count>prior) s.circuit.cell[prior].flags=1;
            retain(s.memory,{d.in,d.l},{d.out,d.r});s.performance.active=0;++s.revisions;
        } else proposal={};
    }
    out[i]=proposal;
}
#include "procedure_gait.cuh"
ENTRY void procedure_explore(Mind *m,roost::Kinetics *history,const apprenticeship::Convention *english,Word n,Word siblings,Word seed,Word style,Word steps) {
    Word i=group()*32+lane();if (i>=n) return;Word key=mix(seed+(i%siblings)*0x9e3779b9u);
    auto v=*english;apprenticeship::dialect(v,key,style);
    roost::explore(m[i].muscle,history[i],apprenticeship::foray(v,key,style),roost::climate(key^0xa38cda71u),mix(key+2741),steps);
}
ENTRY void procedure_attend(Mind *m,roost::Kinetics *history) {
    Word i=group();__attribute__((shared)) static Word votes[32];
    roost::attend(m[i].muscle,history[i],m[i].genes,lane(),votes);
}
ENTRY void procedure_gait_score(Mind *m,const apprenticeship::Convention *english,roost::GaitScore *out,Word n,Word siblings,Word seed,Word style,Word survey) {
    Word i=group()*32+lane();if (i>=n) return;Word key=mix(seed+(i%siblings)*0x9e3779b9u);
    auto v=*english;apprenticeship::dialect(v,key,style);auto law=apprenticeship::foray(v,key,style);auto w=roost::climate(key^0xa38cda71u);
    auto &policy=m[i].muscle;roost::GaitScore s;s.training=policy.practice;
    for (Word j=0;j<survey;++j) {
        Word cue=survey==3072 ? j/12 : mix(key+j*5147)&255,want=j%12;
        Word motor=roost::muscle(policy,roost::proprioception(law,want),cue,mix(key+j+195));
        s.correct+=roost::effect(w,cue,motor)==want;++s.decided;
    }
    s.sensor_pair=(policy.first==w.a && policy.second==w.b) || (policy.first==w.b && policy.second==w.a);out[i]=s;
}
ENTRY void procedure_motor_merit(Word *fitness,const roost::GaitScore *score,Word n,Word siblings) {
    Word i=group()*32+lane();if (i>=n) return;Word success=0,count=0;
    for (Word j=0;j<siblings;++j) {success+=score[i*siblings+j].correct;count+=score[i*siblings+j].decided;}
    fitness[i]+=count ? 112*success/count : 0;
}

ENTRY void procedure_listen(Hearing *state,const Count *frames,Word n,RecollectionGenes genes,Count *symbols,Word *count,Word end) {
    Word heard=*count;auto receive=[&](Count wave) {if (heard<32766) symbols[heard++]=wave;else if (heard!=~0u) ++heard;};
    for (Word j=0;j<n;++j) sample(*state,frames[j],genes,receive);
    if (end) release(*state,genes,receive);
    *count=heard;
}
#include "procedure_roost.cuh"
ENTRY void procedure_mentor(Mind *child,const Mind *adult,RecollectionGenes genes,Word n,Word seed,Word cut) {
    Word i=group()*32+lane();if (i>=n) return;
    child[i]={};child[i].genes=genes;roost::rear_from(child[i],*adult,mix(seed+i),cut);
}
ENTRY void roost_founders(roost::Genome *parents,roost::Terrain *ground,Word n,RecollectionGenes genes,Word seed) {
    Word i=group()*32+lane();if (i>=n) return;Word k=mix(seed+i*0x9e3779b9u);
    parents[i]=roost::mutate({genes,mix(k),mix(k+1),mix(k+2),i,0,1},mix(k+3));
    if (!lane()) ground[i/32]={roost::season(~0ull,k)&(Count(mix(k+7))|(Count(mix(k+8))<<32)),mix(seed+i/32),0};
}
ENTRY void roost_rear(Mind *pupils,const Mind *adults,const roost::Genome *genes,const apprenticeship::Convention *base,Word n,Word seed,Word style,Word rounds,Word established,Word cultural) {
    Word i=group()*32+lane();if (i>=n) return;
    auto &p=pupils[i];p={};p.genes=genes[i].learning;if (!genes[i].alive) return;
    Word key=mix(seed^((i/32)*0x9e3779b9u)),weather=mix(key+i);auto d=*base;apprenticeship::dialect(d,key,style);
    if (established && cultural!=2) {if (cultural!=1) roost::rear_from(p,adults[i/32],mix(31*seed+i),0);}
    else {
        auto hear=[&](Slice x,Slice y){apprenticeship::listen(p,x,y,weather,0);};
        apprenticeship::show(d,key,rounds,hear);apprenticeship::routes(d,key,style,false,hear);
    }
}
ENTRY void roost_body(Mind *m,roost::Kinetics *history,const apprenticeship::Convention *base,Word n,Word seed,Word epoch,Word style,Word steps) {
    Word i=group()*32+lane();if (i>=n) return;Word key=mix(seed^((i/32)*0x9e3779b9u));auto v=*base;apprenticeship::dialect(v,key,style);
    roost::explore(m[i].muscle,history[i],apprenticeship::foray(v,key,style),roost::climate(mix(key+epoch)),mix(seed+epoch*0xb80c443fu+i),steps);
}
ENTRY void roost_forage(Mind *m,const roost::Genome *genes,roost::Terrain *fields,const apprenticeship::Convention *base,roost::Day *out,
                        Word seed,Word epoch,Word style,Word days,Word arm,Word invention,Word storm,roost::Kinetics *books,Word version) {
    Word t=lane(),i=group()*32+t;auto &s=m[i];auto g=genes[i];roost::Day day;day.energy=g.alive ? 8*days+24 : 0;
    alignas(roost::Message) __attribute__((shared)) static Byte message_data[sizeof(roost::Message)*32];
    auto *mail=reinterpret_cast<roost::Message *>(message_data);
    auto land=fields[group()];Word key=mix(seed^(group()*0x9e3779b9u)),home=0;auto d=*base;
    apprenticeship::dialect(d,key,style);auto law=apprenticeship::foray(d,key,style);auto w=roost::climate(mix(key+epoch));
    Count buds=land.buds,previous=0;Word streak=0;auto &book=books[i];
    bool adaptive=version>=2 && arm!=4 && arm!=6;
    bool test_candidate=version>=3 && arm!=7;book.n=book.since=book.errors=book.first=book.remaining=0;book.span=32u<<((g.fecundity>>26)&3);
    __attribute__((shared)) static Word votes[32];
    for (Word tick=0;tick<days;++tick) {
        Word started=day.moves,adhered=day.reactive;
        if (storm && tick==storm) w=roost::climate(mix(key+epoch+779));
        Word season_key=mix(land.seed^(tick+land.visits)*0x2e8170b1u),draw=mix(season_key+i*0x44671b49u);
        Word route=mix(g.roaming+((tick/(2+((g.roaming>>11)&7)))%4)*9211+epoch);
        Count goals[roost::movements];Word steps=day.energy ? roost::intended(s,route,goals,roost::movements) : 0;
        if (day.energy && !steps) {steps=4;for (Word k=0;k<steps;++k) goals[k]=roost::proprioception(law,mix(draw+k)%12);}
        auto flight=roost::fly(s,g,law,w,buds,home,land.seed,draw,goals,steps,arm,day,adaptive ? &book : nullptr);
        if (!steps && day.sorties) {--day.sorties;--day.landings;}
        auto &offer=mail[t];offer.n=offer.lesson=0;offer.steps=flight.steps;
        Count digest=0;for (Word k=0;k<flight.steps;++k) {offer.deed[k]=flight.echo[k];digest=(digest^flight.echo[k])*radix;}
        streak=digest==previous ? streak+1 : 1;previous=digest;
        if (day.energy && flight.patch!=64 && flight.end==home) {
            offer.n=roost::narrate(s,flight.echo,flight.steps,offer.voice,roost::words);
            if (offer.n>2 && invention && streak>=2 && ((g.voice>>16)&1) && day.innovations<4) {
                Word named=roost::coin(s,mix(route+g.voice+day.innovations),flight,offer.voice);
                if (named) {offer.n=named;offer.lesson=1;++day.innovations;}
            }
            if (offer.n) {++day.utterances;day.spikes+=offer.n;day.abbreviated+=offer.n<=2;}
        }
        if (!group() && flight.steps && (!day.path_n || (flight.patch!=64 && flight.end==home))) {
            day.path[0]=static_cast<Byte>(home);day.path_n=static_cast<Byte>(1+flight.steps);day.path_tick=tick;
            day.fruit=static_cast<Byte>(flight.patch);day.call=offer.n ? offer.voice[0] : 0;
            for (Word j=0;j<flight.steps;++j) day.path[j+1]=flight.path[j];
        }
        Word found=flight.patch;Word self=roost::elect(day.energy && flight.end==home && steps ? found : 64+t,t,season_key);
        day.meals+=self;
        asm volatile("bar.warp.sync 0xffffffff;":::"memory");
        const auto &heard=mail[t^1];bool audible=heard.n && arm!=1;
        Word policy=Word(audible)+2*Word(found<64)+4*Word(day.energy<days);
        bool listen=audible && ((g.voice>>policy)&1);
        if (audible && day.energy) ++day.receptions;
        Word count=0;
        if (listen && day.energy) {
            Count call[roost::words];for (Word j=0;j<heard.n;++j) call[j]=heard.voice[arm==2 ? heard.n-1-j : j];
            if (heard.lesson && ((g.voice>>17)&1) && invention) day.imitations+=roost::named(s,call,heard.n,heard.deed,heard.steps,false);
            count=roost::interpret(s,call,heard.n,goals,roost::movements,arm!=5);
            if (count) {++day.understood;bool match=count==heard.steps;for (Word j=0;j<count && match;++j) match=goals[j]==heard.deed[j];day.matched+=match;}
        }
        if (!count && day.energy) count=roost::intended(s,mix(route+1729),goals,roost::movements);
        if (!count && day.energy) {count=3;for (Word j=0;j<count;++j) goals[j]=roost::proprioception(law,mix(draw+j+75)%12);}
        // Each fruit holds two pods. Scouts and returning foragers compete
        // separately for them; both groups draw on the same living field.
        auto return_trip=roost::fly(s,g,law,w,buds,home,land.seed,mix(draw+72),goals,count,arm,day,adaptive ? &book : nullptr);
        if (!count && day.sorties) {--day.sorties;--day.landings;}
        Word helped=roost::elect(day.energy && return_trip.end==home && count ? return_trip.patch : 96+t,t,mix(season_key+17));
        day.meals+=helped;day.site=return_trip.end;
        Count used=(self ? 1ull<<found : 0)|(helped ? 1ull<<return_trip.patch : 0);buds=roost::season(buds&~roost::gathered(used),season_key);
        Word fee=(steps+count+3)/4+(offer.n+7)/8+(offer.lesson ? (offer.steps+3)/4 : 0);
        if (storm) {
            Word phase=tick<storm ? 0 : tick<storm+4 ? 1 : 2;
            if (phase!=2 || tick+4>=days) {day.attempts[phase]+=day.moves-started;day.success[phase]+=day.reactive-adhered;}
        }
        if (adaptive) {
            Word need=1+((g.fecundity>>20)&15);
            Word cue=Word(__builtin_popcount(book.errors)>=need)+2*Word(book.n==book.span)+4*Word(day.energy<days)+8*Word(book.since>=book.span);
            bool revise=day.energy && !book.remaining && book.n>=16 && book.since>=8+((g.fecundity>>16)&15) && ((g.fecundity>>cue)&1);
            Word requests;asm volatile("{.reg .pred p;setp.ne.u32 p,%1,0;vote.sync.ballot.b32 %0,p,0xffffffff;}":"=r"(requests):"r"(Word(revise)));
            Word old_a=s.muscle.first,old_b=s.muscle.second;
            asm volatile("bar.warp.sync 0xffffffff;":::"memory");
            while (requests) {
                Word selected=Word(__builtin_ctz(requests)),resident=group()*32+selected;requests&=requests-1;
                roost::attend(m[resident].muscle,books[resident],genes[resident].learning,t,votes,false,test_candidate);
                asm volatile("bar.warp.sync 0xffffffff;":::"memory");
            }
            if (revise) {
                ++day.replays;
                if (test_candidate) book.remaining=8+((g.fecundity>>12)&31);
                else day.rewired+=(old_a!=s.muscle.first || old_b!=s.muscle.second);
                fee+=1+book.n/64;book.since=0;
            }
        }
        day.charged+=fee;day.energy=(day.energy>fee ? day.energy-fee : 0)+12*(self+helped);
        asm volatile("bar.warp.sync 0xffffffff;":::"memory");
    }
    day.productions=s.circuit.count;day.receptors=s.ear.size;out[i]=day;if (!t) {land.buds=buds;land.visits+=days;fields[group()]=land;}
}
ENTRY void roost_brood(const roost::Genome *parents,roost::Genome *young,const roost::Day *lives,const Mind *m,Mind *tutors,Word seed,Word drift) {
    Word t=lane(),i=group()*32+t,home=i&~31u,total=0,best=home;Word shares[32];
    for (Word k=0;k<32;++k) {
        shares[k]=parents[home+k].alive ? (drift ? 1u : lives[home+k].energy) : 0;total+=shares[k];
        if (lives[home+k].energy>lives[best].energy) best=home+k;
    }
    if (!t) tutors[group()]=m[best];
    if (!total) {young[i]={};young[i].alive=0;return;}
    Word draw=mix(seed+i*0x9e3779b9u),ticket=draw%total,k=0;
    while (k<31 && ticket>=shares[k]) ticket-=shares[k++];
    young[i]=roost::mutate(parents[home+k],mix(draw+119));
}

ENTRY void procedure_begin(Mind *m,const Decoder *d,const Count *waves,Word n,Node *nodes,Summary *plan,Half *grid,Word *status) {
    m->performance.active=0;
    if (n>performance_span) {*status=capacity;return;}
    Byte input[performance_span];
    for (Word i=0;i<n;++i) {Word id=roost::detect(m->ear,waves[i],m->genes);if (!id) {*status=unrecognized;return;}input[i]=static_cast<Byte>(id);}
    procedure::Tissue t{nodes,plan,grid,performance_span,0,true};*status=prepare(*m,*d,{input,n},t,m->performance);
}
ENTRY void procedure_step(Mind *m,const Decoder *d,Node *node,Summary *plan,Half *grid,Word view,Word open,Motion *out) {
    auto &e=m->performance;*out={};if (!e.active) return;
    if (e.revision!=m->revisions) {e.active=0;out->status=broken;return;}
    procedure::Tissue t{node,plan,grid,performance_span,0,true};Report r;
    if (!read(*m,*d,t,{e.input,e.size},r) || t.n!=e.nodes) {e.active=0;out->status=broken;return;}
    *out=advance(*m,*d,node,e,view,open);
}
ENTRY void procedure_concerts(const Mind *m,const Decoder *d,const apprenticeship::Convention *base,apprenticeship::PerformanceCase *out,
        Word n,Word siblings,Word seed,Word style) {
    Word i=group()*32+lane();if (i>=n) return;Word key=mix(seed+(i%siblings)*0x9e3779b9u);auto v=*base;apprenticeship::dialect(v,key,style);
    out[i]=apprenticeship::concert(m[i],d[i],v,key,style);
}
ENTRY void procedure_contingencies(const Mind *m,const Decoder *d,const apprenticeship::Convention *base,const apprenticeship::PerformanceCase *c,
                                   SceneCheck *score,Word n,Word siblings,Word seed,Word style,Word cases,Word cut) {
    Word i=group(),t=lane();if (i>=n) return;Word key=mix(seed+(i%siblings)*0x9e3779b9u);auto v=*base;apprenticeship::dialect(v,key,style);auto env=apprenticeship::gate_law(key,style);
    SceneCheck local;
    for (Word pair=t;pair<cases;pair+=128) {
        Word raw=cases==65536 ? pair : mix(seed+i*311+pair)&65535,x=raw&255,y=raw>>8;
        bool okay=c[i].valid;auto control=c[i].start;Word k=0,initial=0,first_branch=0;bool differs=false;
        Word join=2;
        for (Word step=0;step<v.length[join];++step) {
            bool complex=v.law[join][step]==call1;Word count=complex ? v.length[0] : 1;
            for (Word inner=0;inner<count;++inner) {
                Word retina=!k ? initial : (k&1 ? x : y),prediction=cut ? initial : retina;
                Word wanted=complex ? v.motor[apprenticeship::opens(env,retina) ? 2 : 1] : v.motor[0];
                if (complex) {if (!first_branch) first_branch=1+wanted;else differs|=first_branch!=1+wanted;}
                Motion motion;if (c[i].valid) motion=advance(m[i],d[i],c[i].nodes,control,prediction,true);
                bool hit=motion.status==moved && motion.emitted==signal(wanted);local.pulses+=hit;++local.expected;okay&=hit;++k;
            }
        }
        if (c[i].valid) okay&=advance(m[i],d[i],c[i].nodes,control,0,true).status==finished;
        local.all+=okay;local.changes+=differs && okay;local.sensitive+=differs;++local.trials;
    }
    __attribute__((shared)) static Word totals[6][128];
    totals[0][t]=local.all;totals[1][t]=local.pulses;totals[2][t]=local.changes;totals[3][t]=local.trials;totals[4][t]=local.expected;totals[5][t]=local.sensitive;
    asm volatile("bar.sync 0;":::"memory");
    for (Word size=64;size;size>>=1) {
        if (t<size) for (Word j=0;j<6;++j) totals[j][t]+=totals[j][t+size];
        asm volatile("bar.sync 0;":::"memory");
    }
    if (!t) score[i]={totals[0][0],totals[1][0],totals[2][0],totals[3][0],totals[4][0],totals[5][0]};
}

ENTRY void procedure_scene_merit(Word *fitness,const SceneCheck *scores,Word n,Word siblings) {
    Word i=group()*32+lane();if (i>=n) return;Word good=0,total=0;
    for (Word k=0;k<siblings;++k) {good+=scores[i*siblings+k].all;total+=scores[i*siblings+k].trials;}
    fitness[i]+=total ? 80*good/total : 0;
}
ENTRY void procedure_call_merit(Word *fitness,const Invention *calls,const Mind *m,Word n) {
    Word i=group()*32+lane();if (i>=n) return;auto &c=calls[i];if (!c.length) return;
    // Score a genuinely shorter utterance whose acquired motor contact still
    // reproduces the complete observed movement.
    for (Word j=0;j<m[i].circuit.count;++j) if (same(cue(m[i].circuit,j),{c.sign,c.length}) && same(output(m[i].circuit,j),{c.movement,c.actions})) {
        fitness[i]+=24+c.saving/(c.exposures ? c.exposures : 1);return;
    }
}

#include "procedure_grounds.cuh"

AX_FN bool match_voice(const Ear &a,Slice one,const Ear &b,Slice two) {
    if (one.n!=two.n) return false;
    for (Word j=0;j<one.n;++j) {
        auto x=one.p[j],y=two.p[j];if (var(x) || var(y)) {if (x!=y) return false;}
        else if (a.model[x]!=b.model[y]) return false;
    }
    return true;
}
ENTRY void procedure_transmitted(const Mind *children,const Mind *teacher,Word n,Word *counts) {
    Word i=group()*32+lane();if (i>=n) return;const auto &m=children[i],&a=*teacher;Word cells=0,loops_seen=0;
    for (Word u=0;u<a.circuit.count;++u) for (Word v=0;v<m.circuit.count;++v) {
        if (a.circuit.cell[u].raw!=m.circuit.cell[v].raw || !match_voice(a.ear,cue(a.circuit,u),m.ear,cue(m.circuit,v))) continue;
        bool equivalent=true;for (Word state=0;state<256 && equivalent;++state) {
            int x=switch_to(a.circuit,u,state,true),y=switch_to(m.circuit,v,state,true);
            equivalent=x<0 || y<0 ? x==y : match_voice(a.ear,part_action(a.circuit,u,x!=0),m.ear,part_action(m.circuit,v,y!=0));
        }
        if (equivalent) {++cells;break;}
    }
    for (Word u=0;u<a.recursion.n;++u) for (Word v=0;v<m.recursion.n;++v) {
        const auto &r=a.recursion.state[u],&s=m.recursion.state[v];
        if (r.n!=s.n || !match_voice(a.ear,{r.prefix,r.head},m.ear,{s.prefix,s.head}) || !match_voice(a.ear,{r.suffix,r.tail},m.ear,{s.suffix,s.tail})
            || !match_voice(a.ear,{r.base,r.center},m.ear,{s.base,s.center})) continue;
        Word matches=0;
        for (Word j=0;j<r.n;++j) for (Word k=0;k<s.n;++k) {
            auto p=r.arm[j],q=s.arm[k];
            if (p.cuts==q.cuts && match_voice(a.ear,token(r,j),m.ear,token(s,k)) &&
                match_voice(a.ear,{r.wire+p.at+p.n,p.m},m.ear,{s.wire+q.at+q.n,q.m})) {++matches;break;}
        }
        if (matches==r.n) {++loops_seen;break;}
    }
    counts[2*i]=cells;counts[2*i+1]=loops_seen;
}
#include "procedure_cohort.cuh"
