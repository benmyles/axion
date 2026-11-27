#include "composition_fit.cuh"
#include "composition_world.cuh"
#include "composition_social.cuh"

using namespace axion;
using namespace axion::life;
using namespace axion::life::composition;
#define ENTRY extern "C" __attribute__((global))

ENTRY void compose_rear(Mind *m,const shelter::Dialect *english,Word count,const RecollectionGenes *genes,Word siblings,Word seed,Word style,Word rounds) {
    Word i=block_id()*32+component();if (i>=count) return;
    auto &animal=m[i];animal={};animal.genes=genes[i/siblings];Word env=i%siblings;auto d=*english;shelter::vary(d,mix(seed+env),style);
    shelter::Physical room;shelter::empty(room);
    auto port=[&](shelter::Frame f) {Mouth x,y;shelter::encounter(room,d,f,x,y);shelter::retain(animal,x,y);};
    shelter::play(port,mix(seed+713+env),rounds,3,false);
}
ENTRY void compose_patterns(Mind *m,Detections *d) {
    Word i=block_id(),lane=component();auto &body=m[i];
    __attribute__((shared)) static Induction s;
    if (!lane) body.anatomy={};
    together();vocabulary(body,s,lane,false);vocabulary(body,s,lane,true);
    if (!lane) {
        for (auto &head:d[i].tail) head=0;
        d[i].n=0;afferents(body,d[i]);
    }
}
ENTRY void compose_induce(Mind *m,const Detections *d) {
    Word block=block_id(),channel=block%width,group=(block/width)%queries,i=block/(width*queries);
    if (group>=m[i].anatomy.phrases_n || channel>=m[i].anatomy.phrases[group].length) return;
    alignas(Evidence) __attribute__((shared)) static unsigned char scratch[sizeof(Evidence)];
    auto &w=*reinterpret_cast<Evidence *>(scratch);
    induce(m[i],d[i],group,channel,w,component());
}
ENTRY void compose_settle(Mind *m,Word n) {Word i=block_id()*32+component();if (i<n) settle(m[i]);}
ENTRY void compose_expose(Mind *m,const Exchange *ports,Word n) {
    for (Word i=0;i<n;++i) shelter::retain(*m,ports[i].heard,ports[i].reply);
}
ENTRY void compose_turn(Mind *m,Exchange *ports,Word n) {
    for (Word i=0;i<n;++i) converse(*m,ports[i],true);
}
ENTRY void compose_trials(Mind *m,const shelter::Dialect *english,Measures *out,Word n,Word seed,Word style,Word rounds,Word depth,Word cut,Word siblings,Word noise) {
    Word i=block_id()*32+component();if (i>=n) return;
    auto &s=m[i];auto &a=s.anatomy;s.memory={};Word env=i%siblings;auto d=*english;shelter::vary(d,mix(seed+env),style);
    for (Word j=0;j<a.phrases_n;++j) for (Word k=0;k<a.phrases[j].length;++k) {
        auto &f=a.phrases[j].voice[k];if (cut==1) {f.repeat=0;f.second=0;}
        if (cut==2) f.gate=0;
        if (cut==3 && f.first && a.fields[f.first-1].right!=absent) f.other=absent;
    }
    if (cut==4) s.genes.low=0;
    shelter::Physical room;shelter::empty(room);Measures result;Word vocabulary=s.lexicon.n,weather=mix(seed+env+3347);
    auto port=[&](shelter::Frame f) {
        Mouth x,y;shelter::encounter(room,d,f,x,y);
        for (Word j=0;j<x.size;++j) for (Word bit=0;bit<noise;++bit) {weather=mix(weather+27449);x.words[j]^=1ull<<(weather&63);}
        auto heard=perceive(s.lexicon,x,s.genes,true);Word handled=0,distance=0;
        auto reply=act(a,s.memory,s.genes,heard,handled,distance);
        if (f.kind<shelter::where) {++result.actions;return;}
        auto predicted=emit(s.lexicon,reply);bool correct=predicted.size==y.size;
        for (Word k=0;k<y.size && correct;++k) correct=predicted.words[k]==y.words[k];
        if (!y.size) {++result.unknown;result.unknown_quiet+=!reply.size;return;}
        ++result.queries;result.correct+=correct;result.voiced+=bool(reply.size);
        if (!correct && !result.missed_at) {
            result.missed_at=result.queries;result.first_input=x;result.expected=y;result.produced=predicted;
        }
        if (f.kind==shelter::believe && room.shut[f.x%12] && shelter::root(room.parent,f.y)!=shelter::root(room.view[f.x%12],f.y)) {
            ++result.displaced;result.displaced_correct+=correct;
        }
        if (f.kind==shelter::where || f.kind==shelter::search) {
            Word v=f.kind==shelter::where ? f.x : room.taste[f.x],links=0;
            while (v!=absent && room.parent[v]!=absent && links<96) {v=room.parent[v];++links;}
            if (links>3) {++result.deep;result.deep_correct+=correct;}
        }
    };
    shelter::roam(port,mix(seed+0x781592a3u+env),rounds,depth);
    port({shelter::where,95});result.fresh=s.lexicon.n-vocabulary;out[i]=result;
}

ENTRY void compose_merit(const Measures *scores,Word *fitness,Word n,Word siblings) {
    Word i=block_id()*32+component();if (i>=n) return;
    Word value=0;
    for (Word k=0;k<siblings;++k) {
        const auto &r=scores[i*siblings+k];value+=16*r.correct+8*r.displaced_correct+4*r.deep_correct+r.unknown_quiet;
    }
    fitness[i]=value;
}
#include "composition_island.cuh"
ENTRY void market_seed(market::Genome *genes,Word n,RecollectionGenes founder,Word seed) {
    Word i=block_id()*32+component();if (i<n) genes[i]=market::offspring({founder,i,0,1},mix(seed+i));
}
ENTRY void market_rear(Mind *m,const market::Genome *genes,const shelter::Dialect *english,Word n,Word seed,Word style,Word rounds) {
    Word i=block_id()*32+component();if (i>=n) return;
    auto &s=m[i];s={};s.genes=genes[i].learner;if (!genes[i].alive) return;
    auto d=*english;shelter::vary(d,mix(seed+i/2),style);
    shelter::Physical world;shelter::empty(world);
    auto port=[&](shelter::Frame f) {Mouth x,y;shelter::encounter(world,d,f,x,y);shelter::retain(s,x,y);};
    shelter::play(port,mix(seed^i*0x63749fabu^249u),rounds,3,false);
}
ENTRY void market_forage(Mind *m,const market::Genome *genes,const shelter::Dialect *english,market::Life *out,
                         Word n,Word seed,Word style,Word days,Word arm) {
    Word pair=block_id()*16+component(),lane=component();
    if (pair>=n/2) return;
    auto &left=m[2*pair],&right=m[2*pair+1];left.memory={};right.memory={};left.pending={};right.pending={};
    auto d=*english;shelter::vary(d,mix(seed+pair),style);shelter::Physical w;shelter::empty(w);
    market::Life tally;auto &a=tally.first,&b=tally.second;a.energy=b.energy=days+8;
    auto reveal=[&](shelter::Frame f,Word audience) {
        Mouth x,y;shelter::encounter(w,d,f,x,y);
        if (audience&1) market::transmit(left,x);
        if (audience&2) market::transmit(right,x);
    };
    for (Word j=0;j<8;++j) reveal({shelter::put,56+j,64+j%4},3);
    for (Word j=0;j<4;++j) reveal({shelter::put,64+j,88+j},3);
    bool present=genes[2*pair].alive && genes[2*pair+1].alive;
    for (Word j=0;j<8;++j) reveal({shelter::choose,48+j,56+(mix(seed+pair*31+j)&7)},j<4 ? 1 : 2);
    for (Word t=0;t<days;++t) {
        Word k=mix(seed+pair*0x91d38badu+t*0x9e3779b9u);
        Word p=48+(k&7),o=56+((k>>3)&7),q=48+((k>>8)&7),r=48+((k>>13)&7);
        reveal({shelter::choose,p,o},p<52 ? 1 : 2);reveal({shelter::put,64+((k>>17)&3),88+((k>>19)&3)},3);
        Word win=0,other=0,prior=a.paid,second=b.paid;
        if (present && a.energy) market::day(left,right,d,w,q,arm,q>=52,a,win);
        if (present && b.energy) market::day(right,left,d,w,r,arm,r<52,b,other);
        // The sixteen pairs compete for nectar on one island. Rotate access
        // priority so a lane's physical position gives no persistent advantage.
        Word bids;asm volatile("{ .reg .pred p; setp.ne.u32 p,%1,0; vote.sync.ballot.b32 %0,p,0xffff; }":"=r"(bids):"r"(win));
        Word bids2;asm volatile("{ .reg .pred p; setp.ne.u32 p,%1,0; vote.sync.ballot.b32 %0,p,0xffff; }":"=r"(bids2):"r"(other));
        Word offers=bids|(bids2<<16),weather=mix(seed+block_id()*419u+t),rotation=weather&31;
        Word ordered=(offers>>rotation)|(offers<<((32-rotation)&31)),cap=12+((weather>>6)&7);
        bool meal=win && Word(__builtin_popcount(ordered&((1u<<((lane+32-rotation)&31))-1)))<cap;
        bool theirs=other && Word(__builtin_popcount(ordered&((1u<<((lane+48-rotation)&31))-1)))<cap;
        Word fee=a.paid-prior,cost=b.paid-second;
        a.energy=(a.energy>fee ? a.energy-fee : 0)+5*Word(meal);b.energy=(b.energy>cost ? b.energy-cost : 0)+5*Word(theirs);
        a.meals+=meal;b.meals+=theirs;
    }
    if (!present) {a.energy=0;b.energy=0;}
    out[pair]=tally;
}
ENTRY void market_brood(const market::Genome *old,market::Genome *young,const market::Life *scores,Word seed,Word drift) {
    Word i=block_id()*32+component(),island=i&~31u,total=0;
    Word mass[32];
    for (Word j=0;j<32;++j) {auto &l=scores[(island+j)/2];mass[j]=old[island+j].alive ? (drift ? 1 : j&1 ? l.second.energy : l.first.energy) : 0;total+=mass[j];}
    if (!total) {young[i]={};young[i].alive=0;return;}
    Word draw=mix(seed+i*0x51633e2du),ticket=draw%total,parent=0;
    while (parent<31 && ticket>=mass[parent]) ticket-=mass[parent++];
    young[i]=market::offspring(old[island+parent],mix(draw+293));
}
ENTRY void compose_mentor(Mind *pupils,Mind *teachers,const Mind *adult,const shelter::Dialect *english,
                         Word n,RecollectionGenes genes,Word seed,Word style,Word rounds) {
    Word i=block_id()*32+component();if (i>=n) return;
    auto &young=pupils[i];young={};young.genes=genes;
    auto &teacher=teachers[i];teacher=*adult;teacher.memory={};teacher.pending={};
    auto d=*english;shelter::vary(d,mix(seed+i),style);
    auto port=[&](shelter::Frame f) {
        Exchange e;e.heard=shelter::pronounce(d,d.input[f.kind],d.names[f.x],f.kind==shelter::veil ? d.words[f.y ? shelter::asleep : shelter::awake] : d.names[f.y],d.names[f.z]);
        converse(teacher,e,false);shelter::retain(young,e.heard,e.reply);
    };
    shelter::play(port,mix(seed+713+i),rounds,3,false);
}
#include "composition_compact.cuh"
ENTRY void compose_deck(Mind *m,const shelter::Dialect *english,Word seed,Word style,Catalog *c,Atlas *atlas) {
    Word i=block_id();auto &s=m[i];auto d=*english;shelter::vary(d,mix(seed+i),style);auto &deck=c[i];deck={};atlas[i]={};
    for (Word j=0;j<catalog_size;++j) {
        Word k=mix(j*0x9e3779b9u+719u),person=48+(k&3),other=48+((k>>2)&3),object=56+((k>>4)&3);
        shelter::Frame f;
        switch (j%8) {
        case 0:f={shelter::put,object,64+((k>>8)&3)};break;
        case 1:f={shelter::put,64+((k>>10)&3),88+((k>>12)&3)};break;
        case 2:f={shelter::choose,person,object};break;
        case 3:f=j&8 ? shelter::Frame{shelter::veil,person,(k>>21)&1} : shelter::Frame{shelter::give,person,other,object};break;
        case 4:f={shelter::believe,person,object};break;
        case 5:f={shelter::where,object};break;
        case 6:f={shelter::search,person};break;
        default:f=j&8 ? shelter::Frame{shelter::offered,person,other} : shelter::Frame{shelter::follow};break;
        }
        auto mouth=shelter::pronounce(d,d.input[f.kind],d.names[f.x],f.kind==shelter::veil ? d.words[f.y ? shelter::asleep : shelter::awake] : d.names[f.y],d.names[f.z]);
        auto event=perceive(s.lexicon,mouth,s.genes,true);Word a=fire(s.anatomy,event),b=stimulus(s.anatomy,event);
        deck.turns[deck.n++]={event,b<patterns && a==queries ? static_cast<Byte>(b) : absent,a<queries ? static_cast<Byte>(a) : absent};
    }
    pack_layout(atlas[i],s.anatomy,deck);
}
ENTRY void compose_compact(const Mind *brains,const Catalog *decks,const Atlas *atlas,Word *store,Discourse *state,Word *out,Word n,Word models,Word first,Word ticks) {
    Word i=block_id()*128+component();if (i>=n) return;Word id=i%models;
    auto &m=brains[id];auto &d=decks[id];Packed p{atlas[id],store,i,n};
    if (!first) for (Word k=0;k<p.t.words();++k) store[k*n+i]=0;
    Word random=mix(i+971),hash=first ? out[i] : 0;Discourse history=first ? state[i] : Discourse{};
    // Advance the reproducible stimulus stream without replaying any neural state.
    Word jump=first,mul=1664525u,add=1013904223u;
    while (jump) {if (jump&1) random=mul*random+add;add*=1+mul;mul*=mul;jump>>=1;}
    for (Word k=first;k<first+ticks;++k) {
        auto &e=d.turns[select_turn(k,random,d.n)];hash=digest(hash,spoken(m.anatomy,p,history,m.genes,e));
    }
    state[i]=history;out[i]=hash;
}
ENTRY void compose_general(const Mind *brains,const Catalog *decks,Connections<slots> *memory,Word *out,Word n,Word models,Word ticks) {
    Word i=block_id()*128+component();if (i>=n) return;Word id=i%models;
    auto &m=brains[id];auto &d=decks[id];auto &w=memory[i];
    w.n=w.time=w.full=0;w.prior={};w.spoken={};for (auto &v:w.heads) v=0;
    Word random=mix(i+971),hash=0;
    for (Word k=0;k<ticks;++k) {
        auto &e=d.turns[select_turn(k,random,d.n)];Word handled=0,depth=0;
        hash=digest(hash,act(m.anatomy,w,m.genes,e.words,handled,depth));
    }
    out[i]=hash;
}
ENTRY void compose_equivalence(const Mind *brains,const Catalog *decks,const Atlas *atlas,Word *store,Connections<slots> *memory,
                               Word *out,Word n,Word models,Word ticks) {
    Word i=block_id()*128+component();if (i>=n) return;Word id=i%models;
    auto &m=brains[id];auto &d=decks[id];auto &w=memory[i];w={};
    Packed p{atlas[id],store,i,n};for (Word k=0;k<p.t.words();++k) store[k*n+i]=0;
    Discourse history;Word random=mix(i+971),wrong=0,voiced=0,checks=0;
    for (Word k=0;k<ticks;++k) {
        auto &e=d.turns[select_turn(k,random,d.n)];Word handled=0,depth=0;
        auto x=act(m.anatomy,w,m.genes,e.words,handled,depth),y=spoken(m.anatomy,p,history,m.genes,e);
        wrong+=!equal(x,y);voiced+=x.size!=0;
        for (Word q=0;q<p.t.n;++q) {
            Word key=p.t.keys[q],field=key>>16,a=key&255,b=(key>>8)&255;
            auto direct=Awake<slots>{w}.get(field,a,b,w.time),compact=p.get(field,a,b,~0u);
            wrong+=direct.value!=compact.value;++checks;
            if (!compact.time || !(p.t.close[field-1][compact.value/32]&(1u<<(compact.value&31)))) continue;
            for (Word v=0;v<p.t.n;++v) {
                Word other=p.t.keys[v],link=other>>16;
                if (!(m.anatomy.dated&(1ull<<(link-1)))) continue;
                wrong+=Awake<slots>{w}.get(link,other&255,(other>>8)&255,direct.time).value!=p.get(link,other&255,(other>>8)&255,compact.time).value;++checks;
            }
        }
    }
    out[3*i]=wrong;out[3*i+1]=voiced;out[3*i+2]=checks;
}
#include "composition_merge.cuh"
ENTRY void compose_apprentice(const Mind *m,Mind *p) {
    *p={};p->genes=m->genes;p->lexicon=m->lexicon;
    Word pending=m->lessons.seen-m->consolidated;
    p->lessons.n=pending<m->lessons.n ? pending : m->lessons.n;p->lessons.seen=p->lessons.n;
    for (Word i=0;i<p->lessons.n;++i) p->lessons.event[i]=m->lessons.event[m->lessons.n-p->lessons.n+i];
}
ENTRY void compose_absorb(Mind *m,const Mind *p,Word *status) {
    // Construct the candidate topology separately. A capacity failure cannot
    // partially overwrite a previously acquired dialect.
    auto result=m->anatomy;*status=merge(result,p->anatomy);
    if (*status) return;
    m->anatomy=result;m->pending={};++m->revisions;
    bool grounded=true;
    for (Word j=0;j<p->lessons.n;++j) {
        const auto &e=p->lessons.event[j];Word consumed=0,depth=0;
        auto y=act(m->anatomy,m->memory,m->genes,e.stimulus,consumed,depth);
        if (e.response.size && !equal(y,e.response)) grounded=false;
    }
    if (grounded) m->consolidated=m->lessons.seen;
}

ENTRY void compose_calibrate(Mind *m,const Mouth *input,Word n,Catalog *d,Atlas *t) {
    *d={};*t={};
    for (Word k=0;k<n;++k) {
        auto x=perceive(m->lexicon,input[k],m->genes,true);
        if (x.size!=input[k].size) {t->unsupported=1;return;}
        Word q=fire(m->anatomy,x),f=stimulus(m->anatomy,x);
        d->turns[d->n++]={x,q==queries && f<patterns ? static_cast<Byte>(f) : absent,q<queries ? static_cast<Byte>(q) : absent};
    }
    pack_layout(*t,m->anatomy,*d);
}
ENTRY void compose_stream(Mind *m,const Atlas *t,Word *data,Discourse *history,Exchange *e,Word export_state) {
    auto x=perceive(m->lexicon,e->heard,m->genes,false);
    for (Word k=0;k<x.size;++k) if (!x.words[k]) {e->status=3;return;}
    Word q=fire(m->anatomy,x),type=stimulus(m->anatomy,x);
    Stimulus s{x,q==queries && type<patterns ? static_cast<Byte>(type) : absent,q<queries ? static_cast<Byte>(q) : absent};
    if (s.type!=absent) {
        Count fields=m->anatomy.used;
        while (fields) {
            Word j=__builtin_ctzll(fields);fields&=fields-1;auto f=m->anatomy.fields[j];if (f.type!=s.type) continue;
            if (!t->slot(address(j+1,x.words[f.left],f.right==absent ? 0 : x.words[f.right])) || !t->encode[j][x.words[f.out]]) {e->status=3;return;}
        }
    }
    auto y=spoken(m->anatomy,Packed{*t,data,0,1},*history,m->genes,s);
    if (export_state) {
        Word handled=0,depth=0;auto normal=act(m->anatomy,m->memory,m->genes,x,handled,depth);
        if (!equal(normal,y) || m->memory.full) {e->status=5;return;}
    }
    e->reply=emit(m->lexicon,y);
    e->status=s.query!=absent ? 2 : s.type!=absent ? 1 : 0;
}
#include "composition_census.cuh"
ENTRY void compose_census_inputs(Mind *m,const shelter::Dialect *english,Square *r,Word seed,Word style) {
    auto d=*english;shelter::vary(d,mix(seed),style);prepare(*r,*m,d);
}
ENTRY void compose_census(const Mind *m,const Square *r,Enumeration *out,Word first,Word count) {
    Word i=block_id()*128+component();if (i<count) out[first+i]=enumerate(*m,*r,first+i);
}
#include "composition_inquiry.cuh"
ENTRY void compose_contest(const Mind *m,const Detections *d,Competition *r) {
    Word block=block_id(),channel=block%width,group=(block/width)%queries,i=block/(width*queries);
    if (group>=m[i].anatomy.phrases_n || channel>=m[i].anatomy.phrases[group].length) return;
    alignas(Evidence) __attribute__((shared)) static unsigned char memory[sizeof(Evidence)];
    __attribute__((shared)) static Word ranking[alternatives-1];
    compete(m[i],d[i],group,channel,r[i].voice[group][channel],*reinterpret_cast<Evidence *>(memory),ranking,component());
}
ENTRY void compose_question(Mind *models,Detections *d,Competition *pop,Tuition *cost,const shelter::Dialect *english,
                           Mind *teachers,const Mind *adult,Word n,Word first,Word seed,Word style,Word budget,Word wander,const Tuition *yoked) {
    Word i=block_id()*32+component();if (i>=n) return;
    auto &m=models[i];auto &r=pop[i];auto &contacts=d[i];for (auto &head:contacts.tail) head=0;contacts.n=0;
    auto dialect=*english;shelter::vary(dialect,mix(seed+first+i),style);
    shelter::Physical world;shelter::empty(world);Tuition fees;
    if (adult) {teachers[i]=*adult;teachers[i].memory={};teachers[i].pending={};}
    InquiryPort learner{m,r,contacts,fees,yoked ? yoked[i].asked : budget,mix(seed+first+i+201u),0,wander,yoked ? yoked[i].heard : 0};
    auto port=[&](shelter::Frame f) {
        Mouth x,y;
        if (adult) {
            Exchange e;e.heard=shelter::pronounce(dialect,dialect.input[f.kind],dialect.names[f.x],
                f.kind==shelter::veil ? dialect.words[f.y ? shelter::asleep : shelter::awake] : dialect.names[f.y],dialect.names[f.z]);
            converse(teachers[i],e,false);x=e.heard;y=e.reply;
        } else shelter::encounter(world,dialect,f,x,y);
        learner.hear(x,y);
    };
    // Calibration uses the nursery's side of the vocabulary and at most
    // three containment links. The deployment side is disjoint.
    shelter::roam(port,mix(seed+first+i+0x92d0f15du),384,2,0);
    for (Word g=0;g<m.anatomy.phrases_n;++g) {
        auto &q=m.anatomy.phrases[g];if (!q.ready) continue;
        for (Word k=0;k<q.length;++k) {
            auto &set=r.voice[g][k];Word winner=__builtin_ctz(set.alive);q.voice[k]=set.f[winner];fees.changed+=winner!=0;
        }
    }
    settle(m);cost[i]=fees;
}
ENTRY void compose_purpose(const Mind *m,const Mouth *question,const Mouth *outcome,Mouth *move,Word *trials) {
    Voices ears=m->lexicon;
    const auto cue=perceive(ears,*question,m->genes,false),wanted=perceive(ears,*outcome,m->genes,false);
    *trials=0;*move=emit(ears,synthesise(*m,cue,wanted,*trials));
}
