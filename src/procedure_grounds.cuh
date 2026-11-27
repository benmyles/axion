// Device entry points for composing the learned motor model with the learned
// score. Only physical execution and the independent audit receive an apparatus.
#include "procedure_pack.cuh"
#include "garden_motion.cuh"
#include "causal_world.cuh"
#include "lantern.cuh"
ENTRY void procedure_plant(const Mind *m,const Agency *a,MotorModel *p,Word allowed) {
    Word t=lane();
    for (Word j=t;j<=letters;j+=128) {
        Word tag=j<=m->ear.size ? volition::lookup(a->speech,acoustic::unwave(m->ear.model[j])) : 0;
        p->actuator[j]=meaning::unique(tag) && (tag&allowed&volition::motor_mask) ? static_cast<Byte>(__builtin_ctz(tag)) : unknown_state;
    }
    for (Word j=t;j<operations*local_states;j+=128) {
        auto v=causal::predict(a->model,j/local_states,j%local_states);
        p->next[j/local_states][j%local_states]=v.unknown ? unknown_state : static_cast<Byte>(v.on);
    }
}
ENTRY void procedure_projection(const Mind *m,const Decoder *d,const MotorModel *p,const Node *node,const Performance *plan,Transfer *out) {
    Word t=lane();
    for (Word i=1;i<=plan->nodes;++i) {
        auto hit=project(*m,*d,*p,node[i],out,{plan->input,plan->size},t);
        out[i].destination[t]=static_cast<Byte>(hit.state);out[i].pulses[t]=hit.pulses;
        asm volatile("bar.sync 0;":::"memory");
    }
}
ENTRY void procedure_situate(const Mind *m,const Decoder *d,const Count *waves,Word n,Node *nodes,Summary *sums,Half *grid,Performance *plan,Word *status) {
    *plan={};*status=capacity;if (!n || n>performance_span) return;
    Byte input[performance_span];
    for (Word i=0;i<n;++i) {Word id=roost::detect(m->ear,waves[i],m->genes);if (!id) {*status=unrecognized;return;}input[i]=static_cast<Byte>(id);}
    procedure::Tissue t{nodes,sums,grid,performance_span,0,true};*status=prepare(*m,*d,{input,n},t,*plan);
}
ENTRY void procedure_settle(const Mind *m,const Decoder *d,const MotorModel *p,const Node *node,const Performance *script,
                           const Transfer *root,Agency *a,Concepts *c,Agency *b,Concepts *other,Apparatus *w,GardenFocus f,GroundReply *reply) {
    *reply={};auto &out=*reply;Word site=f.site;
    if (f.name) {Word noun=meaning::noun(*c,f.name);if (!meaning::unique(noun)) {out.stop=1;return;}site=__builtin_ctz(noun);}
    if (site>=32 || !(w->present&(1u<<site)) || !(f.eyes&1) || !(a->observed&(1u<<site))) {out.stop=1;return;}
    out.start=a->scenes[site];out.expected=root->destination[out.start];out.predicted_moves=root->pulses[out.start];out.motion.subject=site;
    if (!f.execute) return;
    if (out.expected<local_states && out.predicted_moves>4096) {out.stop=4;return;}
    auto pc=*script;Word seen=0;
    for (;;) {
        if (!(a->observed&(1u<<site))) {out.stop=1;return;}
        auto pulse=advance(*m,*d,node,pc,a->scenes[site],true);
        if (pulse.status==finished) return;
        if (pulse.status!=moved) {out.stop=2;return;}
        Word id=roost::detect(m->ear,pulse.emitted,m->genes),motor=p->actuator[id];
        if (motor>=operations) {out.stop=3;return;}
        if (seen++==4096) {out.stop=4;return;}
        volition::move(a,c,b,other,*w,site,motor,f.eyes,out.motion);
    }
}
ENTRY void procedure_physical_oracle(const Mind *m,const Decoder *d,const MotorModel *p,const Node *node,const Performance *script,Word seed,Word lamp,Word limit,Transfer *out) {
    Word t=lane();auto garden=lamp ? lantern::cabinet(seed) : greenhouse::garden(seed);auto pc=*script;Word state=t;
    for (;;) {
        auto hit=advance(*m,*d,node,pc,state,true);if (hit.status==finished) break;
        if (hit.status!=moved || pc.moves>limit) {state=unknown_state;break;}
        Word id=roost::detect(m->ear,hit.emitted,m->genes),a=p->actuator[id];
        if (a>=operations) {state=unknown_state;break;}
        state=causal::physical(garden,a,state);
    }
    out->destination[t]=static_cast<Byte>(state);out->pulses[t]=pc.moves;
}
ENTRY void procedure_palette(const Mind *m,const MotorModel *p,Palette *v,Word senses) {
    Word t=lane();if (!t) inventory(*m,*p,*v,senses);
    asm volatile("bar.sync 0;":::"memory");
    for (Word i=0;i<v->size;++i) {Word a=chosen(*m,v->choice[i],t);v->fate[i][t]=a<operations ? p->next[a][t] : unknown_state;}
}
ENTRY void procedure_search(const Mind *m,const Palette *p,Word length,Word choices,Count goal,Count starts,Count *winning) {
    Word t=lane(),l=t&31,w=t/32,block=group();Count best=0;Word stride;
    asm("mov.u32 %0, %%nctaid.x;":"=r"(stride));
    for (Word id=block*4+w;id<choices;id+=stride*4) {
        Word code=id,spelling=length>1 ? (m->circuit.cell[p->join].input-2)*(length-1) : 0;
        Word low=l,high=l+32;
        for (Word k=0;k<length;++k) {
            Word pick=code%p->size;code/=p->size;spelling+=p->choice[pick].cost;
            if (low<local_states) low=p->fate[pick][low];
            if (high<local_states) high=p->fate[pick][high];
        }
        Word a=meaning::ballot(low<local_states && ((goal>>low)&1) && ((starts>>l)&1));
        Word b=meaning::ballot(high<local_states && ((goal>>high)&1) && ((starts>>(l+32))&1));
        if (!l && spelling<=performance_span) {
            Count rank=(Count(__builtin_popcount(a)+__builtin_popcount(b))<<56)|(Count(255-length)<<48)|(Count(65535-spelling)<<32)|(~id);
            if (rank>best) best=rank;
        }
    }
    __attribute__((shared)) static Count partial[4];if (!l) partial[w]=best;asm volatile("bar.sync 0;":::"memory");
    if (!t) {for (Word i=1;i<4;++i) if (partial[i]>best) best=partial[i];
        asm volatile("atom.global.max.u64 %0,[%1],%2;":"=l"(best):"l"(winning),"l"(best):"memory");}
}
ENTRY void procedure_pronounce(const Mind *m,const Palette *v,Word id,Word stages,InferredPlan *out) {
    Byte steps[max_steps];for (Word k=0;k<stages;++k) {steps[k]=static_cast<Byte>(id%v->size);id/=v->size;}
    *out=pronounce(*m,*v,steps,stages);
}
ENTRY void procedure_pack(const Transfer *root,CompactTransfer *to,Word *bytes) {
    *bytes=compact(*root,*to);
}
ENTRY void procedure_unpack(const CompactTransfer *p,Transfer *out) {
    Word t=lane();auto hit=procedure::evaluate(*p,t);out->destination[t]=static_cast<Byte>(hit.state);out->pulses[t]=hit.pulses;
}
ENTRY void procedure_travel(const Mind *m,const Decoder *d,const MotorModel *p,const Node *nodes,const Performance *plan,
                           const Transfer *root,const CompactTransfer *compact,Trial *results,Word n,Word rounds,Word seed,Word packed) {
    Word i=group()*128+lane();if (i>=n) return;Word weather=mix(seed+i),state=weather&63;Trial out;
    for (Word k=0;k<rounds;++k) {
        // Each history receives an independent physical disturbance between
        // calls. Guards inside a call always sense the current plant state.
        weather=weather*1664525u+1013904223u;state^=weather>>26;
        if (packed==2) {auto hit=procedure::evaluate(*compact,state);out.movements+=hit.pulses;state=hit.state;}
        else if (packed==1) {out.movements+=root->pulses[state];state=root->destination[state];}
        else {
            auto pc=*plan;
            for (;;) {
                auto hit=advance(*m,*d,nodes,pc,state,true);if (hit.status==finished) break;
                if (hit.status!=moved) {state=unknown_state;break;}
                Word id=roost::detect(m->ear,hit.emitted,m->genes),a=p->actuator[id];
                if (a==unknown_state || (state=p->next[a][state])==unknown_state) break;
            }
            out.movements+=pc.moves;
        }
        if (state>=local_states) break;
        out.digest=(out.digest^state)*16777619u;
    }
    out.state=state;results[i]=out;
}

ENTRY void procedure_desire(const Agency *a,Count sound,Count *target) {
    *target=0;Word category=volition::category(*a,sound);
    if (category<8 && (a->assembly.active&(1u<<category))) {*target=a->assembly.field[category].certain;return;}
    Word meaning=volition::lookup(a->speech,sound);
    if (!meaning::unique(meaning)) return;
    Word cell=__builtin_ctz(meaning);
    if (cell>=6 && cell<18) {auto bit=causal::plane((cell-6)%6);*target=cell<12 ? bit : ~bit;}
}

AX_FN void learn_lantern(CausalModel &m,Word motor,Word seed,Word samples,Word attention,Word priorities,Word *correct) {
    Word t=lane();auto cabinet=lantern::cabinet(seed);
    if (t<traits) m.fields[motor][t]={};
    if (!t) m.visited[motor]=0;
    meaning::warp_fence();Word shuffled=mix(seed+5141*motor),stride=(shuffled>>8)|1,settled_at=65;
    for (Word j=0;j<samples;++j) {
        Word candidate=(j*stride+shuffled)&63;
        if (attention) {
            Word key=mix(shuffled+j),bid=0,unknown=0;
            for (Word s=t;s<local_states;s+=32) {
                auto p=causal::predict(m,motor,s);Word disagreements=__builtin_popcount(p.unknown);
                unknown|=disagreements;
                if (m.visited[motor]&(1ull<<s)) continue;
                Word rank=(1+((priorities>>(4*disagreements))&15))*64+((s+(key&63))&63);
                if (rank>bid) bid=rank;
            }
            if (!meaning::ballot(unknown) && settled_at==65) settled_at=j;
            for (Word step=16;step;step>>=1) {Word other;asm volatile("shfl.sync.bfly.b32 %0,%1,%2,31,0xffffffff;":"=r"(other):"r"(bid),"r"(step));if (other>bid) bid=other;}
            candidate=((bid&63)-(key&63))&63;
        }
        if (!t) causal::learn(m,{candidate,motor,causal::physical(cabinet,motor,candidate)});
        meaning::warp_fence();
    }
    Word hits=0,held=0,settled=0;
    for (Word state=t;state<local_states;state+=32) if (!(m.visited[motor]&(1ull<<state))) {
        auto guess=causal::predict(m,motor,state);hits+=!guess.unknown && guess.on==causal::physical(cabinet,motor,state);settled+=!guess.unknown;++held;
    }
    for (Word step=16;step;step>>=1) {
        hits+=meaning::broadcast(hits,t^step);held+=meaning::broadcast(held,t^step);settled+=meaning::broadcast(settled,t^step);
    }
    if (!t) {correct[4*motor]=hits;correct[4*motor+1]=held;correct[4*motor+2]=settled;correct[4*motor+3]=settled_at;}
}

AX_FN Count preimage(const Palette &p,Word step,Count target,Word l) {
    Word s=p.fate[step][l],r=p.fate[step][l+32];
    Word lo=meaning::ballot(s<local_states && ((target>>s)&1)),hi=meaning::ballot(r<local_states && ((target>>r)&1));return lo|(Count(hi)<<32);
}
ENTRY void procedure_preimages(const Mind *m,const Palette *p,const Suffix *prior,Bid *bids,Count starts,Word width,Word old,BeamStats *stats) {
    Word t=lane(),l=t&31,idx=group()*4+t/32,limit=old*p->size,stride;asm("mov.u32 %0, %%nctaid.x;":"=r"(stride));
    for (;idx<limit;idx+=4*stride) {
        Word parent=idx/p->size,move=idx%p->size;const auto &tail=prior[parent];if (!tail.accepts) continue;
        Count yes=preimage(*p,move,tail.accepts,l);if (l || !yes) continue;
        Word cost=tail.cost+p->choice[move].cost+(tail.length ? m->circuit.cell[p->join].input-2 : 0);
        if (cost>performance_span) continue;
        Word bin=mix(Word(yes)^mix(Word(yes>>32)))&(width-1);
        Count first,value=(Count(__builtin_popcountll(yes&starts))<<56)|(Count(__builtin_popcountll(yes))<<48)|(Count(65535-cost)<<32)|(~idx);
        asm volatile("atom.global.cas.b64 %0,[%1],0,%2;":"=l"(first):"l"(&bids[bin].representative),"l"(yes):"memory");
        if (first && first!=yes) asm volatile("red.global.or.b32 [%0],1;"::"l"(&stats->collision):"memory");
        asm volatile("red.global.max.u64 [%0],%1;"::"l"(&bids[bin].rank),"l"(value):"memory");
    }
}
ENTRY void procedure_suffixes(const Palette *p,const Suffix *prior,const Bid *bids,Suffix *next,Word width,Count *top,BeamStats *stats) {
    Word t=lane(),l=t&31,i=group()*4+t/32;if (i>=width) return;
    Count rank=bids[i].rank;if (!rank) {if (!l) next[i].accepts=0;return;}
    Word index=~Word(rank),parent=index/p->size,move=index%p->size;const auto &tail=prior[parent];
    Count yes=preimage(*p,move,tail.accepts,l);
    if (!l) {
        auto &r=next[i];r.accepts=yes;r.cost=65535-Word((rank>>32)&65535);r.length=tail.length+1;r.step[0]=static_cast<Byte>(move);
        for (Word j=0;j<tail.length;++j) r.step[j+1]=tail.step[j];
        // Among full solutions prefer a shorter spoken witness, then the
        // stable bucket index. The first solved layer has minimum depth
        // among retained suffixes.
        Count quality=(rank&0xff00ffff00000000ull)|(~i);asm volatile("red.global.max.u64 [%0],%1;"::"l"(top),"l"(quality):"memory");
        asm volatile("red.global.add.u32 [%0],1;"::"l"(&stats->occupied):"memory");
    }
}
ENTRY void procedure_utter_policy(const Mind *m,const Palette *p,const Suffix *s,InferredPlan *out) {*out=pronounce(*m,*p,s->step,s->length);}

ENTRY void procedure_lantern(Agency *a,Word seed,Word samples,Word attention,Word priorities,Word *correct) {
    learn_lantern(a->model,group(),seed,samples,attention,priorities,correct);
}
ENTRY void procedure_lantern_merit(const RecollectionGenes *genomes,RecollectionGenes optics,Word seed,Word contacts,Word *scores) {
    __attribute__((shared)) alignas(CausalModel) static Byte storage[sizeof(CausalModel)];
    auto &model=*reinterpret_cast<CausalModel *>(storage);
    Word actor=group()/operations,motor=group()%operations;
    if (!lane()) model.genes=optics;
    meaning::warp_fence();
    learn_lantern(model,motor,seed,contacts,1,genomes[actor].expression,scores+actor*4*operations);
}
ENTRY void procedure_value_questions(const Word *outcomes,Word *fitness,Word n) {
    Word i=group()*32+lane();if (i>=n) return;Word correct=0;
    for (Word a=0;a<operations;++a) correct+=outcomes[(i*operations+a)*4];
    fitness[i]+=correct;
}

ENTRY void procedure_pending(const Mind *m,Count *voice,Word *size) {
    const auto &p=m->performance;Word n=p.active && !p.moves && p.revision==m->revisions ? p.size : 0;
    if (!lane()) *size=n;
    for (Word j=lane();j<n;j+=128) voice[j]=m->ear.model[p.input[j]];
}
ENTRY void procedure_consent(Mind *m,const Agency *a,const Concepts *c,const Count *voice,Word n,Word *ok) {
    *ok=0;if (!m->performance.active || m->performance.moves) return;
    auto intent=volition::listen(*a,*c,voice,n);if (intent.ambiguous) return;
    if (intent.mode==Word(Disposition::plan)) m->performance.active=0;
    *ok=!intent.actions && intent.object==32 && intent.mode==Word(Disposition::act);
}
ENTRY void procedure_survey(const RecollectionGenes sensors,RecollectionGenes physics,Word seed,Word contacts,Word attention,Word *out) {
    __attribute__((shared)) alignas(CausalModel) static Byte storage[sizeof(CausalModel)];
    auto &model=*reinterpret_cast<CausalModel *>(storage);Word world=group()/operations,motor=group()%operations;
    if (!lane()) model.genes=physics;
    meaning::warp_fence();learn_lantern(model,motor,mix(seed+world*0x9e3779b9u),contacts,attention,sensors.expression,out+world*4*operations);
}

ENTRY void procedure_rest(Transfer *row,Performance *plan) {
    Word t=lane();row->destination[t]=static_cast<Byte>(t);row->pulses[t]=0;if (!t) *plan={};
}

// 32 lanes each keep two learned transitions in registers. One shuffle
// routes a private sensory address to its owning lane each simulated call.
ENTRY void procedure_warp_travel(const CompactTransfer *table,Trial *results,Word n,Word rounds,Word seed) {
    Word i=group()*128+lane(),l=lane()&31;if ((i&~31u)>=n) return;
    auto even=procedure::evaluate(*table,2*l),odd=procedure::evaluate(*table,2*l+1);
    Word packed=even.state|(odd.state<<16);
    bool short_counts=table->count_bits<=9;
    if (short_counts) packed|=(Word(even.pulses-table->floor)<<7)|(Word(odd.pulses-table->floor)<<23);
    Word weather=mix(seed+i),state=weather&63;Trial out;
    for (Word k=0;k<rounds;++k) {
        weather=weather*1664525u+1013904223u;state^=weather>>26;
        Word pair=meaning::broadcast(packed,state>>1),row=(pair>>(16*(state&1)))&65535;
        if (short_counts) out.movements+=row>>7;
        else {
            Word a=meaning::broadcast(Word(even.pulses),state>>1),b=meaning::broadcast(Word(odd.pulses),state>>1);
            Word ah=meaning::broadcast(Word(even.pulses>>32),state>>1),bh=meaning::broadcast(Word(odd.pulses>>32),state>>1);
            out.movements+=state&1 ? Count(b)|(Count(bh)<<32) : Count(a)|(Count(ah)<<32);
        }
        state=row&127;out.digest=(out.digest^state)*16777619u;
    }
    if (short_counts) out.movements+=rounds*table->floor;
    out.state=state;if (i<n) results[i]=out;
}
