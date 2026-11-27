// Each earlier receptor can send four efference-specific collaterals to a
// sensory ensemble. The same cells are available to every cue and motor.
AX_FN TraceGenes trace_founder(Word key) {
    return {mix(key^0xae35412bu),0,mix(key^0x5f5cd973u)};
}
AX_FN TraceGenes inherit_trace(TraceGenes p,const Genome &g,Word key,bool mutate,Word &edits) {
    if (!mutate) return p;
    Word words[]{p.plastic,p.charge,p.attention};
    Word chance=4+((g.reproduction>>8)&31);
    UNROLL
    for (Word i=0;i<3;++i) {
        Word draw=mix(key^((i+1)*0x7fd4b513u));
        if ((draw&255)<chance) { words[i]^=1u<<((draw>>8)&31);++edits; }
    }
    return {words[0],words[1],words[2]};
}
#if defined(AX_SEQUENCE)
constexpr bool temporal_image=true;
#else
constexpr bool temporal_image=false;
#endif
template<int Tiles,bool Enabled> struct Temporal {};
template<int Tiles> struct Temporal<Tiles,true> {
    static constexpr int rows=32/Tiles;
    TraceGenes program;
    Word weights[rows],sources[rows],recent,held,conflicts,dirty;
    bool attached=false,adapts=false,routed=false,compares=false;
    AX_FN void attach(const TraceLayer &layer,Word tile,bool fresh,Word mode) {
        attached=true;adapts=mode>=5;compares=mode>=7;program=layer.genes;routed=mode>=6 && (program.attention&(1u<<11));
        recent=fresh ? 0 : layer.recent; held=fresh ? 0 : layer.held;conflicts=fresh ? 0 : layer.conflicts;
        dirty=fresh ? ~0u : 0;
        UNROLL
        for (int j=0;j<rows;++j) {
            weights[j]=fresh ? 0 : layer.predictions[tile+j*Tiles];
            sources[j]=fresh ? 0 : layer.sources[tile+j*Tiles];
        }
    }
    AX_FN void save(TraceLayer &dest,Word tile) const {
        UNROLL
        for (int j=0;j<rows;++j) if (dirty&(1u<<j)) {
            dest.predictions[tile+j*Tiles]=weights[j];dest.sources[tile+j*Tiles]=static_cast<unsigned char>(sources[j]);
        }
        if (!tile) { dest.recent=recent;dest.held=held;dest.conflicts=conflicts; }
    }
    AX_FN Word span() const { return 1+((program.attention&7)%5); }
    AX_FN Word active(bool short_only,bool no_hold,bool no_novelty,bool learning=true) const {
        Word set=0,history=recent;
        for (Word j=0;j<(short_only ? 1 : span()) && history;++j,history>>=6) {
            Word code=history&63;
            if (code) set|=1u<<((code-1)&31);
        }
        Word stable=held&63,rare=(held>>6)&63;
        if (stable && !short_only && !no_hold) set|=1u<<((stable-1)&31);
        if (adapts && rare && !short_only && !no_novelty && (compares || (program.attention&(1u<<(learning ? 27 : 28))))) set|=1u<<((rare-1)&31);
        return set;
    }
    AX_FN Word reliable_contacts(Word bank,Word threshold) const {
        Word charges=(bank>>5)&0x07070707;
        return population((charges+(8-threshold)*0x01010101u)&0x08080808u);
    }
    AX_FN void hear(Word tag,Word tile,Word base,Word mask) {
        Word token=tag+1,same=(recent&63)==token;
        Word local=0;
        UNROLL
        for (int j=0;j<rows;++j) if (tag/Tiles==Word(j)) local=weights[j];
        Word w=shuffle(local,base+(tag&(Tiles-1)),mask);
        Word threshold=1+((program.attention>>9)&3),count=1+((program.attention>>21)&3);
        Word recognized=reliable_contacts(w,threshold)>=count;
        Word support=0;
        if (routed || compares) {
            Word votes=0;
            UNROLL
            for (int j=0;j<rows;++j) {
                Word key=sources[j],afferent=(tile+j*Tiles)^(routed ? trace_offset(program,key&31) : 0);
                if ((key&32) && afferent==tag) {
                    votes|=Word(reliable_contacts(weights[j],threshold)>=count);
                    if (compares) votes|=2u<<(key>>6);
                }
            }
            votes=ensemble_or<Tiles>(votes,tile,base,mask);
            recognized=votes&1;
            if (compares && votes>=2) support=30u-Word(__builtin_clz(votes));
        }
        Word condition=recognized+2*same+4*((conflicts>>tag)&1);
        if ((program.plastic>>(24+condition))&1) held=(held&~63u)|token;
        if (compares) {
            Word threshold=1+((program.attention>>25)&1);
            Word previous=(held>>12)&3;
            if (support>=threshold && (support>=previous || (program.attention&(1u<<26))))
                held=(held&63)|(token<<6)|(support<<12);
        } else if (adapts) {
            // Two otherwise unused bits of each branch retain afferent
            // adaptation. The clock and an optional event share the latch word.
            Word period=4u<<((program.attention>>25)&3),phase=((held>>12)+1)&(period-1);
            Word sensitivity=(program.attention>>29)&1,opposite=(program.attention>>30)&1,mark=0;
            UNROLL
            for (int j=0;j<rows;++j) {
                Word k=sources[j],fatigue=k>>6,next=fatigue-Word(!phase && fatigue);
                if (tile+j*Tiles==tag) { mark=Word(fatigue<=sensitivity)^opposite;next=3; }
                if (next!=fatigue) { sources[j]=(k&63)|(next<<6);dirty|=1u<<j; }
            }
            mark=shuffle(mark,base+(tag&(Tiles-1)),mask);
            if (mark) held=(held&~(63u<<6))|(token<<6);
            held=(held&4095)|(phase<<12);
        }
        if (!same || (program.attention&8)) recent=((recent<<6)|token)&0x3fffffffu;
    }
    AX_FN void credit(Word origin,Word motor,Word target,Word known,Word prior,Word tile,bool labile,bool no_novelty,bool fixed_support) {
        Word bit=1u<<origin,was=Word(bool(conflicts&bit)),mismatch=known && prior!=target;
        Word flag=(program.plastic>>(was+2*known+4*mismatch))&1;
        conflicts=(conflicts&~bit)|(flag<<origin);
        Word enabled=active(false,false,no_novelty);
        if (!(program.attention&(1u<<16))) enabled&=~bit;
        UNROLL
        for (int j=0;j<rows;++j) if (enabled&(1u<<((tile+j*Tiles)^(routed ? trace_offset(program,origin) : 0)))) {
            Word old=weights[j],key=sources[j];
            bool bound=(key&63)==(32+origin);
            if (compares && bound && !fixed_support) {
                Word prediction=(old>>(8*motor))&255;
                if (prediction>>5) {
                    // Replay the two predictions made before the outcome.
                    // Each pair of results has an inherited sign of its own.
                    Word pair=Word((prediction&31)==target)+2*Word(known && prior==target);
                    Word support=key>>6;
                    bool up=bool(program.attention&(1u<<(27+pair)));
                    Word next=up ? support+Word(support<3) : support-Word(support>0);
                    if (next!=support) { key=(key&63)|(next<<6);sources[j]=key;dirty|=1u<<j; }
                }
            }
            if (!((program.plastic>>(8+flag+2*known+4*Word(bound)))&1)) continue;
            // The inherited threshold can protect a well-supported branch
            // from unrelated activity. Contradictory outcomes at its own
            // place still reach the local charge and replacement programs.
            if (!bound && !labile) {
                Word threshold=1+((program.attention>>18)&7);
                if (reliable_contacts(old,threshold)>=1+((program.attention>>23)&3)) continue;
            }
            Word bank=bound ? old : 0,shift=8*motor,value=(bank>>shift)&255;
            Word strength=value>>5,dest=value&31;
            bool present=strength!=0,match=present && dest==target,weak=strength<=((program.attention>>6)&7);
            Word stimulus=Word(present)+2*Word(match)+4*Word(weak);
            int delta=int(((program.charge>>(4*stimulus))&15)^8)-8;
            int charge=int(strength)+delta;
            charge=charge<0 ? 0 : charge>7 ? 7 : charge;
            if ((program.plastic>>(16+Word(weak)+2*Word(match)+4*Word(present)))&1) dest=target;
            Word replacement=(bank&~(255u<<shift))|((Word(charge)*32+dest)<<shift);
            if (replacement!=old || !bound) dirty|=1u<<j;
            weights[j]=replacement;sources[j]=(compares && !bound ? 0u : key&192)|(32+origin);
        }
    }
    // Build the transient graph for this percept. Ordinary transition memory
    // stays intact, so switching a recalled cue can immediately change a plan.
    AX_FN void project(Word (&view)[rows],Word tile,Word base,Word mask,Word destinations,bool short_only,bool no_hold,bool no_novelty) const {
        Word ranks[rows]{},emit=0;
        Word n=short_only ? 1 : span(),minimum=1+((program.attention>>4)&3);
        bool use_rare=adapts && !short_only && !no_novelty && (compares || (program.attention&(1u<<28)));
        Word eligible=active(short_only,no_hold,no_novelty,false);
        if (program.attention&(1u<<17)) destinations&=conflicts;
        UNROLL
        for (int j=0;j<rows;++j) {
            Word row=tile+j*Tiles,key=sources[j],place=key&31,cue=row^(routed ? trace_offset(program,place) : 0);
            if ((key&32) && (destinations&(1u<<place)) && (eligible&(1u<<cue)) && reliable_contacts(weights[j],minimum)) emit|=1u<<row;
        }
        emit=ensemble_or<Tiles>(emit,tile,base,mask);
        while (emit) {
            Word tag=Word(__builtin_ctz(emit));emit&=emit-1;
            Word w=0,k=0;
            UNROLL
            for (int j=0;j<rows;++j) if (tag/Tiles==Word(j)) { w=weights[j];k=sources[j]; }
            Word lane=base+(tag&(Tiles-1));w=shuffle(w,lane,mask);k=shuffle(k,lane,mask);
            Word cue=1+(tag^(routed ? trace_offset(program,k&31) : 0)),history=recent,bias=0;
            // Charge is primary. Equal charge and bias retain temporal order,
            // independently of where these collaterals lie in the warp.
            for (Word i=0;i<n && history;++i,history>>=6) if ((history&63)==cue) {
                bias=((program.attention&(1u<<15)) ? 8*(n-i) : 0)+7-i;break;
            }
            if (!no_hold && !short_only && (held&63)==cue) {
                Word v=8*((program.attention>>12)&7)+7-n;bias=v>bias ? v : bias;
            }
            if (use_rare && ((held>>6)&63)==cue) {
                Word v=64*(program.attention>>31)+6-n;bias=v>bias ? v : bias;
            }
            UNROLL
            for (int j=0;j<rows;++j) if ((k&31)==tile+j*Tiles) {
                UNROLL
                for (Word a=0;a<4;++a) {
                    Word shift=8*a,entry=(w>>shift)&255,strength=entry>>5,target=entry&31;
                    Word rank=8*strength+bias;
                    if (strength<minimum || rank<=((ranks[j]>>shift)&255)) continue;
                    ranks[j]=(ranks[j]&~(255u<<shift))|(rank<<shift);
                    Word sensory=target|32u|(target==(k&31) ? 128u : 64u);
                    view[j]=(view[j]&~(255u<<shift))|(sensory<<shift);
                }
            }
        }
    }
};
