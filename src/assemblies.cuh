// A cell group can be recruited by the consequences it predicted or by
// previously bound ambient spikes. The environment never addresses a group.
AX_FN ContextGenes context_founder(Word seed) {
    Word wires = (mix(seed+53) & 0xfc0fffffu) | (1u<<23);
    return {0xaaaaaaaau, mix(seed+37), mix(seed+41),
            (mix(seed+47) & 0xfec00000u) | (3u<<26) | 0xaaaau, wires};
}
AX_FN ContextGenes inherit_context(ContextGenes parent, const Genome &body, LinkGenes links, Word key,
                                   Word probability, bool mutate, Word &edits) {
    if (!mutate) return parent;
    Word words[5]{parent.plastic,parent.evidence,parent.dynamics,parent.coordination,parent.receptors};
    UNROLL
    for (Word i=0; i<5; ++i) {
        Word x = mix(key ^ (0xc2b2ae35u*(i+17)));
        if ((x & 255) < probability) { words[i] ^= 1u<<((x>>8)&31); ++edits; }
    }
    // Duplication can express an existing germline update program at a new
    // junction. Acquired contact states never enter the germline.
    Word x = mix(key ^ 0x93b1806du);
    if ((x & 511) < probability) {
        Word shift = (x>>5) & 16;
        Word rule = (x & 1024) ? body.expression>>16 : (links.plastic >> ((x>>6)&16)) & 65535;
        Word copy = (words[0] & ~(65535u<<shift)) | (rule<<shift);
        edits += copy != words[0]; words[0] = copy;
    }
    return {words[0],words[1],words[2],words[3],words[4]};
}
AX_FN Word splice_program(Word a,Word b,Word draw) {
    if (!(draw&3)) return a;
    if ((draw&3)==1) return b;
    Word mask=rotate((1u<<((draw>>8)&31))-1,(draw>>16)&31);
    return (a&~mask)|(b&mask);
}
AX_FN ExpressionGenes inherit_expression(ExpressionGenes parent,ContextGenes primary,const Genome &body,LinkGenes links,
                                         Word key,Word chance,bool change,Word &edits) {
    if (!change) return parent;
    Word mask=mix(key^0x6d766ca3u), gate=mix(key^0xf11bbe95u);
    if ((mask&255)<chance) { parent.sensors^=1u<<((mask>>8)&31); ++edits; }
    if ((gate&255)<chance) { parent.gate^=1u<<((gate>>8)&31); ++edits; }
    if ((mix(key^0x39931d27u)&2047)<chance) { parent.alternative=primary; ++edits; }
    parent.alternative=inherit_context(parent.alternative,body,links,key^0x82ac640du,chance,true,edits);
    return parent;
}
enum class ContextCut : Word { intact, pooled, silent, blind, fixed, no_landmarks, no_binding, no_inputs, no_reinstatement, fixed_confidence, labile, external_only, primary_only, alternate_only, swapped };
template<int Tiles, bool Enabled> struct Assemblies {};
template<int Tiles> struct Assemblies<Tiles,true> {
    static constexpr int groups = Tiles < 4 ? 4/Tiles : 1;
    ContextGenes primary;
    ExpressionGenes expression{};
    mutable ContextGenes program;
    Word program_history=0;
    mutable Word transcribed=0;
    bool dual=false;
    mutable ContextCell cell[groups];
    Word raw_flip=0, binding_flip=0, confidence_flip=0;
    Word activity, proposals, landmarks, dirty;
    ContextCut context_cut = ContextCut::intact;
    Word forced_group=4; // explicitly supervised address control in the assay

    AX_FN void configure(Word alternate) const {
        if (context_cut==ContextCut::primary_only) alternate=0;
        if (context_cut==ContextCut::alternate_only) alternate=1;
        // The backing bits describe departures from the developmental state.
        // Change their register representation only when expression switches.
        if (alternate!=transcribed && (raw_flip|binding_flip)) {
            UNROLL
            for (int j=0;j<groups;++j) {
                UNROLL
                for (Word m=0;m<4;++m) cell[j].inputs[m]^=raw_flip;
                cell[j].bindings^=binding_flip;
            }
        }
        transcribed=alternate;
        // Scalar selects keep both inherited programs in registers. Selecting
        // a struct address here makes NVPTX allocate the whole brain on-stack.
        const auto &second=expression.alternative;
        program={alternate ? second.plastic : primary.plastic,
                 alternate ? second.evidence : primary.evidence,
                 alternate ? second.dynamics : primary.dynamics,
                 alternate ? second.coordination : primary.coordination,
                 alternate ? second.receptors : primary.receptors};
    }
    AX_FN Word confidence(Word id) const { return ((proposals>>(24+2*(id&3)))&3) ^ (transcribed ? confidence_flip : 0); }
    AX_FN void sensory_program(Word raw,Word ambient) const {
        if (!dual) return;
        Word selected=raw&expression.sensors, other=raw&~expression.sensors, gate=expression.gate;
        Word flags=Word((raw&(raw-1))!=0) | (Word(selected!=0)<<1) | (Word(other!=0)<<2)
                  | (Word((selected&(selected-1))!=0)<<3) | (((ambient>>((gate>>17)&31))&1)<<4)
                  | (Word(active()!=0)<<5) | (Word((activity>>22&15)!=0)<<6) | (((ambient>>((gate>>22)&31))&1)<<7);
        Word address=((flags>>(gate&7))&1) | (((flags>>((gate>>3)&7))&1)<<1) | (((flags>>((gate>>6)&7))&1)<<2);
        configure(((gate>>(9+address))&1)^Word(context_cut==ContextCut::swapped));
    }
    AX_FN void credit_program(Word lag) const {
        if (dual) configure((program_history>>(lag-1))&1);
    }

    AX_FN void reset_context() {
        program=primary; transcribed=0; program_history=0;
        Word initial = 0u-((program.dynamics>>19)&1);
        Word bindings = ((0u-((program.dynamics>>20)&1)) & 65535u) | ((0u-((program.dynamics>>21)&1)) << 16);
        UNROLL
        for (int i=0; i<groups; ++i) cell[i] = {{initial,initial,initial,initial},bindings};
        activity=0; proposals=((program.receptors>>30)*0x55u)<<24; landmarks=0; dirty=~0u;
        UNROLL
        for (Word i=0;i<8;++i) proposals|=4u<<(3*i);
    }
    AX_FN void load_context(const ContextLayer &s, Word tile, bool fresh,const ExpressionLayer *extra,bool polarity) {
        primary=s.genes; program=primary; transcribed=0; dirty=0; dual=extra!=nullptr;
        if (extra) { expression=extra->genes; program_history=extra->history; }
        if (extra && polarity) {
            Word differences=((primary.dynamics^expression.alternative.dynamics)>>19)&(expression.gate>>27);
            raw_flip=0u-(differences&1);
            binding_flip=((0u-((differences>>1)&1))&65535) | ((0u-((differences>>2)&1))<<16);
            confidence_flip=(expression.gate&(1u<<30)) ? (primary.receptors^expression.alternative.receptors)>>30 : 0;
        }
        if (fresh) return;
        activity=s.activity; proposals=s.proposals; landmarks=s.landmarks;
        UNROLL
        for (int j=0; j<groups; ++j) cell[j] = tile+Tiles*j < 4 ? s.cells[tile+Tiles*j] : ContextCell{};
    }
    AX_FN void store_context(ContextLayer &s, Word tile,ExpressionLayer *extra) const {
        UNROLL
        for (int j=0; j<groups; ++j) if (tile+Tiles*j < 4) {
            auto &dst=s.cells[tile+Tiles*j];
            if (dirty & (1u<<(2*j))) {
                UNROLL
                for (int m=0;m<4;++m) dst.inputs[m]=cell[j].inputs[m]^(transcribed ? raw_flip : 0);
            }
            if (dirty & (2u<<(2*j))) dst.bindings=cell[j].bindings^(transcribed ? binding_flip : 0);
        }
        if (!tile) { s.activity=activity; s.proposals=proposals; s.landmarks=landmarks; }
        if (extra && !tile) extra->history=program_history;
    }
    AX_FN Word ports(Word physical) const {
        return ((physical >> ((program.receptors>>20)&7))&1) | (((physical >> ((program.receptors>>23)&7))&1)<<1);
    }
    AX_FN Word landmark(Word ambient) const {
        Word label=0;
        UNROLL
        for (Word i=0; i<4; ++i) label |= ((ambient >> ((program.receptors>>(5*i))&31))&1)<<i;
        return label;
    }
    AX_FN Word features(Word raw) const {
        Word mode = (program.coordination>>20)&3;
        if (mode==1) return raw & 65535;
        if (mode==2) return raw >> 16;
        if (mode==3 && raw) return 1u<<(mix(raw ^ (program.receptors>>26))&31);
        return raw;
    }
    AX_FN Word afferents(Word raw,Word symbol) const {
        // A burst can recruit an internal motor population onto the very
        // same plastic axons that otherwise receive an external pattern.
        if (context_cut!=ContextCut::external_only && (program.receptors&(1u<<29)) &&
            ((raw&(raw-1)) || (program.receptors&(1u<<26)))) {
            return symbol<4 ? 1u<<(((program.coordination>>16)+symbol*((program.receptors>>4)|1))&31) : 0;
        }
        return features(raw);
    }
    AX_FN Word unique_inputs(const ContextCell &c) const {
        Word a=c.inputs[0], b=c.inputs[1], d=c.inputs[2], e=c.inputs[3];
        return (a|b|d|e) & ~((a&b)|(d&e)|((a|b)&(d|e)));
    }
    AX_FN Word capacity() const { return forced_group<4 ? 4 : context_cut==ContextCut::pooled ? 1 : 1+((program.coordination>>26)&3); }
    AX_FN Word reinstated(Word pattern,Word tile,Word base,Word mask) const {
        Word gain=(program.dynamics>>11)&3;
        if (!gain || context_cut==ContextCut::no_reinstatement) return 0;
        Word support=0, conflict=0;
        UNROLL
        for (int j=0;j<groups;++j) {
            if (tile+Tiles*j>=capacity()) continue;
            const auto &c=cell[j];
            Word distinct=unique_inputs(c), present=pattern&distinct;
            if (!present) continue;
            Word same=0;
            UNROLL
            for (Word m=0;m<4;++m) if (present&c.inputs[m]) same |= c.inputs[m];
            support |= same&distinct; conflict |= ~same&distinct;
        }
        support=ensemble_or<Tiles>(support,tile,base,mask);
        if (program.coordination&(1u<<22)) support &= ~ensemble_or<Tiles>(conflict,tile,base,mask);
        return support&~pattern;
    }
    AX_FN Word indirect(const ContextCell &c,Word concept_code,Word reinstated_inputs) const {
        Word concept_gain = context_cut == ContextCut::no_binding ? 0 : (program.dynamics>>13)&15;
        Word mediated_gain=(program.dynamics>>11)&3;
        Word available=reinstated_inputs ? reinstated_inputs & unique_inputs(c) : 0, out=0;
        UNROLL
        for (Word m=0; m<4; ++m) {
            Word bound = concept_code < 4 && ((c.bindings>>(4*m+(concept_code&3)))&1);
            Word score=bound*concept_gain+Word(population(c.inputs[m]&available))*mediated_gain;
            out |= score << (8*m);
        }
        return out;
    }
    AX_FN Word strengths(const ContextCell &c,Word pattern,Word mediated,Word id) const {
        Word raw_gain=context_cut==ContextCut::no_inputs ? 0 : (program.dynamics>>9)&3;
        Word sensory=0;
        UNROLL
        for (Word m=0;m<4;++m) sensory |= (Word(population(c.inputs[m]&pattern))*raw_gain)<<(8*m);
        if (program.dynamics&(1u<<30)) {
            Word direct=preference(sensory);
            if (direct && !(direct&(direct-1))) return sensory;
        }
        if (program.coordination&(1u<<28)) {
            Word confidence=this->confidence(id), scaled=0;
            UNROLL
            for (Word m=0;m<4;++m) scaled|=((((mediated>>(8*m))&255)*confidence+1)/3)<<(8*m);
            mediated=scaled;
        }
        return sensory+mediated;
    }
    AX_FN int feedback_drive(Word candidates,Word code,bool gain) const {
        if ((program.dynamics&(1u<<22)) && (candidates&(candidates-1))) candidates=0;
        Word own=1u<<code, address=Word((candidates&own)!=0) | (Word((candidates&~own)!=0)<<1) | (Word(gain)<<2);
        return int(((program.evidence>>(4*address))&15)^8)-8;
    }
    AX_FN Word preference(Word votes) const {
        Word hi=0, winners=0;
        UNROLL
        for (Word m=0; m<4; ++m) {
            Word value=(votes>>(8*m))&255;
            if (value>hi) { winners=0; hi=value; }
            if (value==hi) winners |= 1u<<m;
        }
        return winners==15 ? 0 : winners;
    }
    AX_FN Word abstract_code(const Genome &g, Word base_drive) const {
        Word strongest=0, choices=0;
        UNROLL
        for (Word m=0; m<8; ++m) {
            Word value=(base_drive>>(4*m))&15;
            if (value>strongest) { choices=0; strongest=value; }
            if (value==strongest) choices |= 1u<<ports(effector(g,m));
        }
        return strongest && !(choices&(choices-1)) ? Word(__builtin_ctz(choices)) : 4;
    }
    // Reinstated motor activity can itself become a presynaptic event in
    // another assembly. The reference and fallback wiring are inherited.
    AX_FN Word internal_spike(Word cortical, Word raw, Word tile, Word base, Word mask) const {
        Word source=(program.dynamics>>28)&3;
        if (!source) return cortical;
        Word reference=(program.coordination>>24)&3, local=0, pattern=features(raw);
        UNROLL
        for (int j=0;j<groups;++j) {
            Word id=tile+Tiles*j;
            if (id>=capacity() || (source!=3 && id!=reference)) continue;
            Word votes=0;
            UNROLL
            for (Word m=0;m<4;++m) votes |= Word(population(cell[j].inputs[m]&pattern))<<(8*m);
            Word firing=preference(votes);
            if (firing && !(firing&(firing-1))) {
                Word score=16+4*((3+reference-id)&3)+Word(__builtin_ctz(firing));
                if (score>local) local=score;
            }
        }
        UNROLL
        for (int d=1;d<Tiles;d*=2) { Word other=shuffle(local,base+(tile^d),mask); if (other>local) local=other; }
        return local ? local&3 : source==1 ? 4 : cortical;
    }
    AX_FN Word active() const { return (activity>>20)&3; }
    AX_FN int potential(Word id) const { return int(((activity>>(5*(id&3)))&31)^16)-16; }
    AX_FN Word rank(Word id, int charge, Word cue, const ContextCell &group) const {
        if (id>=capacity()) return 0;
        bool same = id==active(), used = (activity>>(22+id))&1;
        Word cue_gain = context_cut==ContextCut::no_landmarks ? 0 : (program.dynamics>>5)&15;
        int drive = 32 + charge + int(same*((program.dynamics>>2)&7)) + int(((group.bindings>>(16+cue))&1)*cue_gain);
        Word tie = (Word(same)<<4) | (Word(!used)<<3) | ((id+((program.coordination>>28)&3))&3);
        return (Word(drive)<<8) | (tie<<2) | id;
    }
    AX_FN Word winner(Word r, Word tile, Word base, Word mask) const {
        if (forced_group<4) return forced_group;
        UNROLL
        for (int d=1;d<Tiles;d*=2) { Word other=shuffle(r,base+(tile^d),mask); if (other>r) r=other; }
        return r&3;
    }
    AX_FN Word express(const Genome &g, Word raw, Word base_drive, Word ambient, Word tile, Word base, Word mask,
                       Word *stamp,const MotorCounts *cortex,Word tie,Word stored_code) const {
        if (context_cut==ContextCut::silent && !stamp) return base_drive;
        Word core=stored_code<4 ? stored_code : (cortex && (program.receptors&(1u<<31))) ?
            ports(effector(g,select_motor<true>(g,*cortex,tie,base_drive))) : abstract_code(g,base_drive);
        Word cue=landmark(ambient), symbol=internal_spike(core,raw,tile,base,mask), local=0;
        UNROLL
        for (int j=0;j<groups;++j) {
            Word id=tile+Tiles*j, r=rank(id,potential(id),cue,cell[j]);
            if (r>local) local=r;
        }
        Word selected=winner(local,tile,base,mask), pattern=afferents(raw,symbol), votes=0;
        if (stamp) *stamp = symbol | (cue<<4) | (selected<<8) | (transcribed<<10);
        if (context_cut==ContextCut::silent) return base_drive;
        if ((program.receptors&(1u<<27)) && selected==((program.coordination>>24)&3)) return base_drive;
        Word recalled=reinstated(pattern,tile,base,mask);
        UNROLL
        for (int j=0;j<groups;++j) if (selected/Tiles==Word(j))
            votes=strengths(cell[j],pattern,indirect(cell[j],symbol,recalled),selected);
        votes=shuffle(votes,base+(selected & (Tiles-1)),mask);
        Word preferred=preference(votes);
        if (!preferred) return base_drive;
        Word floor=255;
        UNROLL
        for (Word m=0;m<4;++m) floor=lesser(floor,(votes>>(8*m))&255);
        Word gain=1+((program.dynamics>>24)&15), residual=(program.dynamics>>17)&3;
        Word drive=0;
        UNROLL
        for (Word m=0;m<8;++m) {
            Word code=ports(effector(g,m)), value=((votes>>(8*code))&255)-floor;
            if ((program.dynamics & (1u<<31)) && !(preferred&(1u<<code))) value=0;
            Word basal=(base_drive>>(4*m))&15;
            drive |= lesser(15u,value*gain+(residual==3 ? 0 : basal>>residual))<<(4*m);
        }
        return drive;
    }
    AX_FN void commit_context(Word stamp) {
        if (dual) program_history=((program_history<<1)|((stamp>>10)&1))&255;
        proposals=(proposals&0xff000000u)|((proposals<<3)&0x00ffffffu)|(stamp&7);
        landmarks=(landmarks<<4)|((stamp>>4)&15);
        activity=(activity & ~(3u<<20)) | (((stamp>>8)&3)<<20);
    }
    AX_FN Word experience(const Genome &g, Word raw, Word motor, bool gain, Word lag, Word tile) {
        Word base=(index()&31)&~(Tiles-1), mask=Word((1ull<<Tiles)-1)<<base;
        Word symbol=(proposals>>(3*(lag-1)))&7, cue=(landmarks>>(4*(lag-1)))&15;
        Word pattern=afferents(raw,symbol), code=ports(effector(g,motor)), next=0, local=0;
        Word recalled=reinstated(pattern,tile,base,mask);
        int reliability[groups]{};
        UNROLL
        for (int j=0;j<groups;++j) {
            Word id=tile+Tiles*j;
            if (id>=capacity()) continue;
            Word mediated=indirect(cell[j],symbol,recalled);
            Word candidates=preference(strengths(cell[j],pattern,mediated,id));
            if ((program.receptors&(1u<<27)) && id==((program.coordination>>24)&3) && symbol<4)
                candidates=1u<<symbol;
            Word suggestion=preference(mediated);
            if (suggestion) reliability[j]=feedback_drive(suggestion,code,gain);
            int evidence=feedback_drive(candidates,code,gain);
            if (context_cut==ContextCut::blind) evidence=0;
            int old=potential(id);
            switch (program.dynamics & 3) {
                case 0: old=0; break;
                case 1: old/=2; break;
                case 2: old=(3*old)/4; break;
            }
            int charge=old+evidence;
            charge=charge < -16 ? -16 : charge>15 ? 15 : charge;
            next |= (Word(charge)&31)<<(5*id);
            Word r=rank(id,charge,cue,cell[j]); if (r>local) local=r;
        }
        Word selected=winner(local,tile,base,mask);
        Word destination=forced_group<4 ? forced_group : ((program.coordination&(1u<<23)) && active()<capacity()) ? active() : selected;
        next=ensemble_or<Tiles>(next,tile,base,mask);
        activity=(activity&0x3c00000u) | next | (selected<<20) | (1u<<(22+destination));
        Word returned=motor;
        if ((program.dynamics&(1u<<23)) && selected!=((program.coordination>>24)&3)) {
            Word row=0;
            UNROLL
            for (int j=0;j<groups;++j) if (selected/Tiles==Word(j)) row=(cell[j].bindings>>(4*code))&15;
            row=shuffle(row,base+(selected&(Tiles-1)),mask);
            returned=8;
            if (row && !(row&(row-1))) {
                Word target=Word(__builtin_ctz(row)), distance=9, actual=effector(g,motor);
                UNROLL
                for (Word m=0;m<8;++m) {
                    Word candidate=effector(g,m), delta=population(candidate^actual);
                    if (ports(candidate)==target && delta<distance) { returned=m; distance=delta; }
                }
            }
        }
        if (context_cut==ContextCut::fixed) return returned;
        Word changed_confidence=0;
        UNROLL
        for (int j=0;j<groups;++j) {
            Word id=tile+Tiles*j;
            if (id>=capacity()) continue;
            ContextCell &c=cell[j];
            Word table=program.coordination >> ((gain ? 8 : 0)+(id==destination ? 4 : 0));
            Word labels=lut2(table,c.bindings>>16,1u<<cue)&65535;
            Word internal=c.bindings&65535;
            Word delta=0;
            if (id==destination) {
                if ((program.coordination&(1u<<28)) && context_cut!=ContextCut::fixed_confidence) {
                    int old=int(confidence(id));
                    int strength=old+(reliability[j]>0)-(reliability[j]<0);
                    strength=strength<0 ? 0 : strength>3 ? 3 : strength;
                    changed_confidence=Word(old^strength)<<(24+2*id);
                }
                UNROLL
                for (Word m=0;m<4;++m) {
                    Word offset=(gain ? 8 : 0)+(m==code ? 4 : 0);
                    Word updated=lut2(program.plastic>>offset,c.inputs[m],pattern);
                    delta|=updated^c.inputs[m]; c.inputs[m]=updated;
                    Word tag=symbol<4 ? 1u<<symbol : 0;
                    Word binding_rule=program.coordination&(1u<<31) ? program.plastic : program.plastic>>16;
                    Word row=lut2(binding_rule>>offset,(c.bindings>>(4*m))&15,tag);
                    internal=(internal & ~(15u<<(4*m))) | ((row&15)<<(4*m));
                }
            }
            Word bindings=internal|(labels<<16);
            dirty|=(Word(delta!=0)+2*Word(bindings!=c.bindings))<<(2*j);
            c.bindings=bindings;
        }
        if (program.coordination&(1u<<28)) proposals ^= ensemble_or<Tiles>(changed_confidence,tile,base,mask);
        return returned;
    }
    AX_FN bool trunk_open() const {
        if ((program.coordination&(1u<<30)) && active()!=((program.coordination>>24)&3)) return false;
        return !(program.coordination&(1u<<(16+active())));
    }
    AX_FN bool context_differs(const Assemblies &other, Word tile) const {
        Word difference=0, raw=(transcribed ? raw_flip : 0)^(other.transcribed ? other.raw_flip : 0);
        Word binding=(transcribed ? binding_flip : 0)^(other.transcribed ? other.binding_flip : 0);
        UNROLL
        for (int j=0;j<groups;++j) if (tile+Tiles*j<4) {
            difference |= cell[j].bindings^other.cell[j].bindings^binding;
            UNROLL
            for (int m=0;m<4;++m) difference |= cell[j].inputs[m]^other.cell[j].inputs[m]^raw;
        }
        return difference!=0;
    }
};
