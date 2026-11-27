#include "sequence_sleep.cuh"
#include "sequence_probe.cuh"
#include "sequence_world.cuh"
#include "sequence_action.cuh"
using namespace axion;
using namespace axion::life;
#define KERNEL extern "C" __attribute__((global))
AX_FN Word thread_index() {
    Word x,b,n;
    asm("mov.u32 %0, %%tid.x;":"=r"(x));
    asm("mov.u32 %0, %%ctaid.x;":"=r"(b));
    asm("mov.u32 %0, %%ntid.x;":"=r"(n));return x+b*n;
}
KERNEL void sequence_sleep(SequenceMind *m,Word n,Word limit) {
    Word t=thread_index(),i=t/32;if (i>=n) return;
    __attribute__((shared)) static sequence::ReplayTissue work;
    sequence::sleep(m[i],work,t&31,limit);
}
KERNEL void sequence_world(Chamber *rooms,Word n,Word family,Word seed,Word states,Word motors,Word sounds,Word unique) {
    Word i=thread_index();if (i<n) rooms[i]=chambers::grow(family,mix(seed+(i%unique)*0x9e3779b9u),states,motors,sounds);
}
KERNEL void sequence_den(Chamber *w,Word n,const Chamber *channels,Word seed) {
    Word i=thread_index();if (i<n) w[i]=chambers::den(*channels,mix(seed+i));
}
KERNEL void sequence_peer_world(Chamber *w,Word n,const SequenceMind *peer) {
    Word i=thread_index();if (i>=n) return;
    Chamber room{};room.states=peer->map.size;room.motors=peer->motors;room.senses=peer->senses;
    for (Word a=0;a<room.motors;++a) room.gestures[a]=peer->gestures[a];
    for (Word o=0;o<room.senses;++o) room.sounds[o]=peer->retina[o].high;
    for (Word q=0;q<room.states;++q) for (Word a=0;a<room.motors;++a) room.rule[q][a]=peer->map.edge[q][a];
    w[i]=room;
}
KERNEL void sequence_born(SequenceMind *m,sequence::Inquiry *inquiry,const Chamber *rooms,
        const RecollectionGenes *genes,Word siblings,Word n,Word seed,DevelopmentLimits limits) {
    Word i=thread_index();if (i>=n) return;
    auto &s=m[i];s={};s.genes=genes[i/siblings];
    for (Word a=0;a<rooms[i].motors;++a) sequence::motor(s,rooms[i].gestures[a],true);
    if (inquiry) sequence::start(inquiry[i],s,mix(seed+(i%siblings)),limits.patience,limits.queries,limits.span,limits.cells);
    else {s.map.size=1;s.active=1;s.anchored=1;}
}
KERNEL void sequence_probe(SequenceMind *m,sequence::Inquiry *inquiry,const Chamber *rooms,
        DevelopmentCost *out,Word count,Word turns,Word noise,const SequenceMind *peer,Word *live) {
    Word i=thread_index();if (i>=count) return;
    auto &s=m[i];auto &b=inquiry[i];if (b.phase==sequence::Phase::done) return;
    sequence::Experiment q;unsigned char observed[sequence::query_length];
    for (Word k=0;k<turns && sequence::propose(b,s,q);++k) {
        Word state=rooms[i].origin,weather=mix(b.questions+b.seed);Count partner=1;
        for (Word j=0;j<q.length;++j) {
            Count pulse;
            if (peer) {
                auto answer=sequence::anticipate(peer->map,partner,q.actions[j]);partner=answer.after;
                pulse=peer->retina[__builtin_ctz(answer.emissions)].high;
                for (Word bit=0;bit<noise;++bit) {weather=mix(weather+0x9e3779b9u);pulse^=1ull<<(weather&63);}
            } else pulse=chambers::experience(rooms[i],state,q.actions[j],weather,noise);
            Word sensed=sequence::recognize(s,pulse,~0ull,true);
            observed[j]=static_cast<unsigned char>(sensed);
            if (sensed<sequence_senses) sequence::record(s.trace,q.actions[j],sensed,j==0);else s.trace.length=0;
        }
        s.events+=q.length;sequence::receive(b,s,q,observed);
    }
    s.active=1;
    out[i]={b.questions,b.steps,b.challenged,b.confirmed,b.rejected};
    if (live && b.phase!=sequence::Phase::done) asm volatile("red.global.or.b32 [%0], %1;"::"l"(live),"r"(1u):"memory");
}
KERNEL void sequence_roam(SequenceMind *m,const Chamber *rooms,DevelopmentCost *out,Word n,Word length,Word seed,Word noise) {
    Word i=thread_index();if (i>=n) return;
    auto &s=m[i];const auto &w=rooms[i];Word random=mix(seed+i),weather=mix(random+17),q=w.origin;
    for (Word k=0;k<length;++k) {
        random=mix(random+0x9e3779b9u);Word a=random%s.motors;
        Word o=sequence::recognize(s,chambers::experience(w,q,a,weather,noise),~0ull,true);
        sequence::observe(s,a,o,k==0);
    }
    out[i]={0,length,0,0,0};
}
// The external port exposes only attempted motor sequences and returned spikes.
KERNEL void sequence_awaken(SequenceMind *m,sequence::Inquiry *inquiry,RecollectionGenes genes,const Count *gestures,
                            Word count,Word seed,DevelopmentLimits limits) {
    *m={};m->genes=genes;
    for (Word a=0;a<count;++a) sequence::motor(*m,gestures[a],true);
    sequence::start(*inquiry,*m,seed,limits.patience,limits.queries,limits.span,limits.cells);
}
KERNEL void sequence_ask(SequenceMind *m,sequence::Inquiry *inquiry,sequence::Experiment *q) {
    if (!sequence::propose(*inquiry,*m,*q)) q->length=0;
}
KERNEL void sequence_hear(SequenceMind *m,sequence::Inquiry *inquiry,const sequence::Experiment *q,const Count *pulses) {
    unsigned char observed[sequence::query_length];
    for (Word j=0;j<q->length;++j) {
        Word s=sequence::recognize(*m,pulses[j],~0ull,true);
        observed[j]=static_cast<unsigned char>(s);
        if (s<sequence_senses) sequence::record(m->trace,q->actions[j],s,j==0);else m->trace.length=0;
    }
    m->events+=q->length;sequence::receive(*inquiry,*m,*q,observed);m->active=1;
}
KERNEL void sequence_port(SequenceMind *body,PulsePort *ports,Word n) {
    auto &m=*body;
    for (Word j=0;j<n;++j) {
        auto &p=ports[j];
        if (p.kind==return_home) {m.active=m.anchored ? 1 : sequence::all(m.map.size);p.active=m.active;continue;}
        if (p.kind==lose_position) {m.active=sequence::all(m.map.size);p.active=m.active;continue;}
        if (p.kind==pursue) {
            Word a=sequence::pursue_wave(m,p.sensation);
            if (a<m.motors) {p.answered=1;p.heard=m.gestures[a];}
            p.active=m.active;continue;
        }
        Word a=sequence::motor(m,p.gesture,p.kind==demonstrate);
        if (p.kind==demonstrate) {
            Word s=sequence::recognize(m,p.sensation,p.visible,true);
            bool error=sequence::observe(m,a,s,p.home);
            if (s<m.senses && a<m.motors) {p.answered=1;
                p.options=error && m.age>=(8u<<((m.genes.expression>>8)&7));}
        } else if (a<m.motors && m.active) {
            auto x=sequence::anticipate(m.map,m.active,a);p.options=sequence::pop(x.emissions);
            if (!x.missing && x.emissions && !(x.emissions&(x.emissions-1))) {
                p.heard=m.retina[__builtin_ctz(x.emissions)].high;p.answered=1;
            }
            m.active=x.missing ? sequence::all(m.map.size) : x.after;
        }
        p.active=m.active;
    }
}

// Selection scores consequences on a new walk through the same physical
// apparatus. Every genome of a generation encounters the same two rooms.
KERNEL void sequence_merit(const SequenceMind *m,const Chamber *rooms,const DevelopmentCost *cost,
                            Word *fitness,Word n,Word siblings,Word seed) {
    Word i=thread_index();if (i>=n) return;
    Word sum=0,expense=0;
    for (Word k=0;k<siblings;++k) {
        Word subject=i*siblings+k;const auto &s=m[subject];const auto &w=rooms[subject];
        Word pos=w.origin,model=0,draw=mix(seed+k*17);
        for (Word j=0;j<192;++j) {
            if (!(j&15)) {pos=w.origin;model=0;}
            draw=mix(draw+0x9e3779b9u);Word a=draw%w.motors,real=w.rule[pos][a];pos=cell_of(real);
            Word prediction=s.map.edge[model][a];if (!prediction) {model=0;continue;}
            model=cell_of(prediction);
            Word missed=sequence::pop(w.sounds[sense_of(real)]^s.retina[sense_of(prediction)].high);
            sum+=64-missed;
        }
        expense+=cost[subject].moves;
    }
    fitness[i]=sum*100u+(100u-(expense<25000 ? expense/250 : 99u));
}
#include "sequence_compact.cuh"
KERNEL void sequence_pack(const SequenceMind *m,sequence::PackedGraph form,Word *bank,Word replicas,Word models) {
    Word id=thread_index();if (id>=replicas) return;const auto &body=m[id%models];
    for (Word j=0;j<form.words();++j) bank[j*replicas+id]=0;
    for (Word cell=0;cell<form.states;++cell) for (Word a=0;a<form.motors;++a) {
        Word e=cell<body.map.size ? body.map.edge[cell][a] : 0;if (!e) continue;
        Word slot=cell*form.motors+a,width=form.width(),offset=slot*width,shift=offset&31;
        Word value=cell_of(e)|(sense_of(e)<<form.cell_bits);
        if (width) bank[(offset/32)*replicas+id]|=value<<shift;
        if (shift+width>32) bank[(1+offset/32)*replicas+id]|=value>>(32-shift);
        if (form.known_words) bank[(form.payload_words+slot/32)*replicas+id]|=1u<<(slot%32);
    }
}
KERNEL void sequence_check_pack(const SequenceMind *m,sequence::PackedGraph form,const Word *bank,Word replicas,Word models,Word *bad) {
    Word i=thread_index();if (i>=replicas) return;
    const auto &body=m[i%models];Word wrong=0;
    for (Word cell=0;cell<form.states;++cell) for (Word a=0;a<form.motors;++a)
        wrong+=(cell<body.map.size ? body.map.edge[cell][a] : 0)!=sequence::load_arc(form,bank,i,replicas,cell,a);
    bad[i]=wrong;
}
KERNEL void sequence_run_packed(sequence::PackedGraph form,const Word *bank,Word *digests,Word replicas,Word ticks,Word salt,Word stored) {
    Word i=thread_index();if (i>=replicas) return;
    Word record=i%stored,cell=0,digest=0,stream=mix(i^salt);
    for (Word j=0;j<ticks;++j) {
        stream=1664525u*stream+1013904223u;Word a=Word((Count(stream)*form.motors)>>32);
        Word e=sequence::load_arc(form,bank,record,stored,cell,a);
        digest=(digest^e)*0x01000193u;cell=e ? cell_of(e) : 0;
    }
    digests[i]=digest;
}
KERNEL void sequence_run_shared(const SequenceMind *m,Word *digests,Word replicas,Word ticks,Word salt,Word stored) {
    Word i=thread_index();if (i>=replicas) return;
    Word record=i%stored,cell=0,digest=0,stream=mix(i^salt);
    for (Word j=0;j<ticks;++j) {
        stream=1664525u*stream+1013904223u;Word a=Word((Count(stream)*m[record].motors)>>32);
        Word e=m[record].map.edge[cell][a];digest=(digest^e)*0x01000193u;cell=e ? cell_of(e) : 0;
    }
    digests[i]=digest;
}
#include "sequence_chorus.cuh"
KERNEL void chorus_founders(chorus::Heredity *pool,Word n,RecollectionGenes inherited,Word seed) {
    Word i=thread_index();if (i>=n) return;
    Word k=mix(seed+i*0x9e3779b9u);
    pool[i]=chorus::mutate({inherited,mix(k),mix(k+1),mix(k+2),mix(k+3),i,0,1},mix(k+4));
}
KERNEL void chorus_cradle(const chorus::Heredity *pool,SequenceMind *m,chorus::Season *life,Word n,Word seed,Word play,Word days,Word arm) {
    Word i=thread_index();if (i>=n) return;
    auto &s=m[i];chorus::hatch(s,pool[i],seed);
    auto &l=life[i];l={};if (!pool[i].alive) return;
    Word key=mix(seed+i),phase=0;
    const auto &other=pool[i^1];
    for (Word j=0;j<play;++j) {
        key=mix(key+0x9e3779b9u);Word a=key&3,o=chorus::voice(other,phase,a);
        Count hearing=chorus::sound(seed,arm==1 ? mix(key+147)&3 : o);
        Word echo=sequence::recognize(s,hearing,~0ull,true);
        sequence::observe(s,a,echo,!j);
    }
    l.partner=phase;l.energy=days/2+4;
}
KERNEL void chorus_feed(const chorus::Heredity *pool,SequenceMind *m,chorus::Season *life,Word seed,Word first,Word days,Word arm) {
    Word i=thread_index(),lane=i&31,island=i/32;
    auto &s=m[i];auto &l=life[i];const auto &g=pool[i],&partner=pool[i^1];
    for (Word step=first;step<first+days;++step) {
        Word weather=mix(seed^island*0x6acfd217u^step*0x9e3779b9u);
        Word key=mix(weather+i*0x56b527ddu);
        // The same pollen scent persists across several motor decisions.
        Word crop=mix(seed+island*0x942b1dd7u+(step/8)*0xe3a117cdu)&3;
        Word gesture=key&3,out=4;
        if (l.energy && partner.alive) {
            Word cue=(l.previous!=0)+2*(l.energy<=12)+4*(sequence::pop(s.active)==1);
            if (arm!=2 && ((g.appetite>>cue)&1)) {
                Word goal=sequence::pursue_wave(s,chorus::sound(seed,crop));
                if (goal<s.motors) {gesture=goal;++l.aims;}
            }
            auto forecast=sequence::anticipate(s.map,s.active,gesture);
            out=chorus::voice(partner,l.partner,gesture);
            Count heard=chorus::sound(seed,arm==1 ? mix(key+147)&3 : out);
            Word observed=sequence::recognize(s,heard,~0ull,true);
            l.accurate+=!forecast.missing && observed<s.senses && forecast.emissions==(1u<<observed);
            sequence::observe(s,gesture,observed,false);++l.echoes;--l.energy;++l.attempts;
        }
        Word eligible=sequence::vote(out==crop),rotation=weather&31;
        Word order=(eligible>>rotation)|(eligible<<((32-rotation)&31));
        Word pos=(lane+32-rotation)&31,quota=3+((weather>>17)&7);
        bool fed=out==crop && Word(__builtin_popcount(order&((1u<<pos)-1)))<quota;
        l.previous=fed;
        if (fed) {++l.meals;l.energy+=6;}
    }
}
KERNEL void chorus_brood(const chorus::Heredity *old,chorus::Heredity *young,const chorus::Season *life,Word seed,Word drift) {
    Word i=thread_index(),base=i&~31u,draw=mix(seed^i*0xda3e39cbu),total=0;
    for (Word j=0;j<32;++j) if (old[base+j].alive) total+=drift ? 1 : life[base+j].energy;
    if (!total) {young[i]={};young[i].alive=0;return;}
    Word ticket=draw%total,p=0;
    for (;p<31;++p) {
        Word mass=!old[base+p].alive ? 0 : drift ? 1 : life[base+p].energy;
        if (ticket<mass) break;
        ticket-=mass;
    }
    young[i]=chorus::mutate(old[base+p],mix(draw+93));
}
KERNEL void chorus_repertoire(const chorus::Heredity *pool,chorus::Repertoire *voices,Word n) {
    Word i=thread_index();if (i<n) voices[i]=chorus::repertoire(pool[i]);
}
