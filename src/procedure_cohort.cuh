// Each family acquires a different physical model. Only acoustic bindings and
// the spoken score are shared across families.
ENTRY void procedure_family_learn(const MotorModel *bindings,RecollectionGenes body,RecollectionGenes sense,MotorModel *models,
                                  Word seed,Word contacts,Word *audit) {
    Word i=group()/operations,a=group()%operations,t=lane(),key=mix(seed+i*0x9e3779b9u);
    __attribute__((shared)) alignas(CausalModel) static Byte storage[sizeof(CausalModel)];
    auto &m=*reinterpret_cast<CausalModel *>(storage);if (!t) m.genes=body;meaning::warp_fence();
    learn_lantern(m,a,key,contacts,1,sense.expression,audit+i*4*operations);
    auto &p=models[i];if (!a) for (Word k=t;k<=letters;k+=32) p.actuator[k]=bindings->actuator[k];
    for (Word x=t;x<local_states;x+=32) {auto v=causal::predict(m,a,x);p.next[a][x]=v.unknown ? unknown_state : static_cast<Byte>(v.on);}
}
ENTRY void procedure_family_compile(const Mind *m,const Decoder *d,const Performance *plan,const Node *node,const MotorModel *models,
                                    Transfer *scratch,Transfer *roots,Byte *pool,Word *offset,Word *cursor,Word stride,Word first) {
    Word batch=group(),t=lane(),i=first+batch;auto *rows=scratch+Count(batch)*stride;
    for (Word j=1;j<=plan->nodes;++j) {
        auto v=project(*m,*d,models[i],node[j],rows,{plan->input,plan->size},t);
        rows[j].destination[t]=static_cast<Byte>(v.state);rows[j].pulses[t]=v.pulses;
        asm volatile("bar.sync 0;":::"memory");
    }
    auto &root=rows[plan->stack[0].node];roots[i].destination[t]=root.destination[t];roots[i].pulses[t]=root.pulses[t];
    if (!t) {
        CompactTransfer p;Word bytes=compact(root,p),at;
        asm volatile("atom.global.add.u32 %0,[%1],%2;":"=r"(at):"l"(cursor),"r"(bytes):"memory");offset[i]=at;
        for (Word j=0;j<bytes;++j) pool[at+j]=reinterpret_cast<const Byte *>(&p)[j];
    }
}
ENTRY void procedure_family_audit(const Mind *m,const Decoder *d,const Performance *plan,const Node *nodes,const MotorModel *models,
                                  const Transfer *roots,const Byte *pool,const Word *offset,Word seed,Word *results) {
    Word i=group(),state=lane(),start=state;auto world=lantern::cabinet(mix(seed+i*0x9e3779b9u));auto pc=*plan;
    auto packed=procedure::evaluate(*reinterpret_cast<const CompactTransfer *>(pool+offset[i]),state);
    bool settled=roots[i].destination[state]<local_states,match=packed.state==roots[i].destination[state] && (!settled || packed.pulses==roots[i].pulses[state]);
    for (;;) {
        auto v=advance(*m,*d,nodes,pc,state,true);if (v.status==finished) break;
        if (v.status!=moved || pc.moves>1024) {state=unknown_state;break;}
        Word id=roost::detect(m->ear,v.emitted,m->genes),action=models[i].actuator[id];
        if (action>=operations) {state=unknown_state;break;}state=causal::physical(world,action,state);
    }
    results[i*local_states+start]=Word(match)+2*Word(settled)+4*Word(state==packed.state && state<local_states && pc.moves==packed.pulses);
}
ENTRY void procedure_family_travel(const Mind *m,const Decoder *d,const Performance *plan,const Node *nodes,const MotorModel *models,
                                   const Transfer *roots,const Byte *pool,const Word *offset,Trial *out,Word count,Word families,Word rounds,Word seed,Word mode) {
    Word i=group()*128+lane();if (i>=count) return;
    Word weather=mix(seed+i),family=mix(i+84521)%families,state=weather&63;Trial r;
    auto &packed=*reinterpret_cast<const CompactTransfer *>(pool+offset[family]);auto &root=roots[family];auto &plant=models[family];
    for (Word k=0;k<rounds;++k) {
        weather=weather*1664525u+1013904223u;state^=weather>>26;
        if (mode==2) {auto hit=procedure::evaluate(packed,state);r.movements+=hit.pulses;state=hit.state;}
        else if (mode==1) {r.movements+=root.pulses[state];state=root.destination[state];}
        else {
            auto pc=*plan;
            for (;;) {
                auto hit=advance(*m,*d,nodes,pc,state,true);if (hit.status==finished) break;
                if (hit.status!=moved) {state=unknown_state;break;}
                Word id=roost::detect(m->ear,hit.emitted,m->genes),a=plant.actuator[id];
                if (a>=operations || (state=plant.next[a][state])>=local_states) break;
            }
            r.movements+=pc.moves;
        }
        if (state>=local_states) break;
        r.digest=(r.digest^state)*16777619u;
    }
    r.state=state;out[i]=r;
}
