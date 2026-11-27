// Descending demand spikes and ascending sensations meet at mutable contacts.
// The private reaction of the plant is available only after the animal acts.
enum class OutcomeCut : Word { intact, silent, fixed, no_association, direct, shuffled, no_demand, no_taste, innate };

AX_FN Word pouch_space(Word stores,Word kind,Word body) {
    Word cap=16+((body>>2)&31), held=(contents(stores)>>(8*kind))&255;
    return held<cap ? cap-held : 0;
}
AX_FN Word metabolize(Word &stores,Word body,Word space) {
    Word bundles=lesser(1+((body>>14)&3),space/4);
    UNROLL
    for (Word k=0;k<4;++k) bundles=lesser(bundles,(stores>>(8*k))&255);
    stores-=bundles*0x01010101u;
    return 4*bundles;
}
AX_FN Word release_energy(Word &stores,Word body,Word space,Word &carbon,bool dietary) {
    if (!dietary) return metabolize(stores,body,space);
    Word n=lesser(1+((body>>14)&3),lesser(space,carbon)/4), ratio=0;
    UNROLL
    for (Word k=0;k<4;++k) {
        Word extra=(body>>(8+k))&1;
        n=lesser(n,((stores>>(8*k))&255)>>extra);
        ratio|=(1u<<extra)<<(8*k);
    }
    stores-=n*ratio; carbon-=4*n;
    return 4*n;
}
AX_FN Word divide_stocks(Word stores,Word reproduction) {
    stores=contents(stores);
    Word share=0, fraction=3+((reproduction>>16)&3);
    UNROLL
    for (Word k=0;k<4;++k) share|=(((stores>>(8*k))&255)*fraction/10)<<(8*k);
    return share;
}
AX_FN Word mouthful(Word body,Word husk) {
    Word size=7+(body&15);
    return husk ? 1+((size-1)>>((body>>28)&3)) : size;
}
AX_FN Word handling_time(Word meal,Word husk) {
    return husk && meal ? lesser(3,(meal-1)>>(5-husk)) : 0;
}
AX_FN OutcomeGenes outcome_founder(Word draw) {
    return {0xaaaaaaaau,mix(draw^0x457c3bb9u),mix(draw^0x6776b835u),mix(draw^0xc40a57efu)};
}
AX_FN OutcomeGenes inherit_outcomes(OutcomeGenes p,const Genome &g,Word key,bool mutate,Word &edits) {
    if (!mutate) return p;
    Word probability=4+((g.reproduction>>8)&31);
    Word words[4]{p.plastic,p.wiring,p.valuation,p.gates};
    UNROLL
    for (Word i=0;i<4;++i) {
        Word r=mix(key^(0x83b1d55du*(i+11)));
        if ((r&255)<probability) { words[i]^=1u<<((r>>8)&31); ++edits; }
    }
    Word r=mix(key^0xc92b5bcdu);
    if ((r&511)<probability) {
        Word shift=(r>>9)&16, source=(r&(1u<<17)) ? g.expression>>16 : (words[0]>>(16-shift))&65535;
        Word next=(words[0]&~(65535u<<shift))|(source<<shift);
        edits+=next!=words[0]; words[0]=next;
    }
    Word reroute=mix(key^0x31a781d3u);
    if ((reroute&511)<probability) {
        Word next=(words[1]&~63u)|((reroute>>9)&63);
        edits+=next!=words[1]; words[1]=next;
    }
    return {words[0],words[1],words[2],words[3]};
}
template<int Tiles> struct Prospects {
    static constexpr int count=(16+Tiles-1)/Tiles;
    Word rows[count], connections, dirty=0;
    OutcomeGenes gene;
    OutcomeCut cut=OutcomeCut::intact;
    AX_FN void load(const OutcomeLayer &source,Word tile,bool fresh) {
        gene=source.genes;
        connections=fresh ? (gene.gates&65535) : source.associations&65535;
        UNROLL
        for (int i=0;i<count;++i) {
            Word row=tile+i*Tiles;
            rows[i]=row<16 ? (fresh ? 0u-((gene.wiring>>30)&1) : source.afferents[row]) : 0;
        }
        dirty=fresh ? ~0u : 0;
    }
    AX_FN void store(OutcomeLayer &dest,Word tile) const {
        UNROLL
        for (int i=0;i<count;++i) if (tile+i*Tiles<16 && (dirty&(1u<<i))) dest.afferents[tile+i*Tiles]=rows[i];
        if (!tile && (dirty&0x80000000u)) dest.associations=(dest.associations&0xffff0000u)|connections;
    }
    AX_FN Word axons(Word raw) const {
        Word low=raw&65535,high=raw>>16,mode=(gene.wiring>>6)&7;
        if (!low || !high || mode<2) return raw;
        if (mode==2) return low;
        if (mode==3) return raw&0xffff0000u;
        Word left=Word(__builtin_ctz(low)),right=Word(__builtin_ctz(high));
        Word shift=(gene.wiring>>9)&3;
        Word address=(mode&1 ? right+(left<<shift) : left+(right<<shift));
        if (mode&2) address=mix(address^(gene.wiring>>14));
        return 1u<<(address&31);
    }
    AX_FN Word channel(Word physical) const {
        return ((physical>>(gene.wiring&7))&1)|(((physical>>((gene.wiring>>3)&7))&1)<<1);
    }
    AX_FN void observe(const Genome &g,Word cue,Word motor,Word flavour,Word nutrient,Word tile) {
        if (cut==OutcomeCut::fixed || cut==OutcomeCut::innate) return;
        Word sensed=(gene.gates&(1u<<30)) && nutrient ? nutrient : flavour;
        if (sensed && motor<8 && cue) {
            Word pattern=axons(cue), code=channel(effector(g,motor));
            UNROLL
            for (int i=0;i<count;++i) {
                Word row=tile+i*Tiles;
                if (row>=16) continue;
                Word shift=4*Word((row>>2)==code)+8*Word((row&3)+1==sensed);
                Word next=lut2(gene.plastic>>shift,rows[i],pattern);
                dirty|=Word(next!=rows[i])<<i; rows[i]=next;
            }
        }
        if (flavour && nutrient && cut!=OutcomeCut::no_association) {
            // A simultaneous flavour and gut pulse can modify this second junction
            // without a motor discharge. Four complete Boolean tables are inherited.
            Word flag=1u<<(flavour-1), next=0, table=gene.plastic>>16;
            UNROLL
            for (Word k=0;k<4;++k) {
                Word shift=(k+1==nutrient ? 4 : 0)+((gene.wiring&(1u<<28)) ? 0 : 8);
                Word updated=lut2(table>>shift,(connections>>(4*k))&15,flag);
                next|=(updated&15)<<(4*k);
            }
            dirty|=Word(next!=connections)<<31; connections=next;
        }
    }
    AX_FN Word forecast(Word cue,Word tile,Word base,Word mask) const {
        Word x=axons(cue), bits=0;
        UNROLL
        for (int i=0;i<count;++i) {
            Word row=tile+i*Tiles;
            if (row>=16) continue;
            Word excited=(gene.wiring&(1u<<31)) ? ~rows[i] : rows[i];
            bool hit=(gene.wiring&(1u<<29)) ? (x && (excited&x)==x) : (excited&x)!=0;
            bits|=Word(hit)<<row;
        }
        return ensemble_or<Tiles>(bits,tile,base,mask);
    }
    struct Currents {
        int predicted, overlap, unknown, familiar;
        AX_FN int integrate(Word prediction,Word demand,bool known) const {
            return predicted*int(population(prediction))+overlap*int(population(prediction&demand))+(known ? familiar : unknown);
        }
    };
    AX_FN Currents currents(Word demand) const {
        // Expand the inherited two-input truth table in the Boolean basis.
        // The constant and demand-only terms cancel in motor competition.
        int w[4];
        UNROLL
        for (Word i=0;i<4;++i) w[i]=int(((gene.valuation>>(4*i))&15)^8)-8;
        Word shift=16+8*Word((demand&(demand-1))!=0);
        int extra=int(gene.gates>>31);
        return {w[1]-w[0]-extra,w[3]-w[1]-w[2]+w[0]+2*extra,
                int(((gene.valuation>>shift)&15)^8)-8,int(((gene.valuation>>(shift+4))&15)^8)-8};
    }
    AX_FN int current_for(Word sensed,Word demand,Word direct,Word indirect,Currents routing) const {
        Word expected=0;
        UNROLL
        for (Word k=0;k<4;++k) expected|=Word((sensed&((connections>>(4*k))&15))!=0)<<k;
        return int(direct)*routing.integrate(sensed,demand,sensed!=0)+int(indirect)*routing.integrate(expected,demand,sensed!=0);
    }
    AX_FN Word motor_current(Word physical,Word score,Word original) const {
        Word value=score*((gene.gates>>20)&3), leak=(gene.gates>>22)&3;
        if ((gene.gates&(1u<<27)) && !(physical&(1u<<((gene.gates>>24)&7)))) value=0;
        return lesser(15,value+(leak==3 ? 0 : original>>leak));
    }
    AX_FN Word drive(const Genome &g,Word cue,Word demand,Word basal,Word tile,Word base,Word mask) const {
        if (!demand || cut==OutcomeCut::silent || cut==OutcomeCut::no_demand) return basal;
        Word map=forecast(cue,tile,base,mask), direct=(gene.gates>>16)&3, indirect=(gene.gates>>18)&3;
        if (cut==OutcomeCut::no_association) indirect=0;
        if (cut==OutcomeCut::direct) { direct=1; indirect=0; }
        auto routing=currents(demand);
        Word scores=0;
        if constexpr (Tiles>=4) {
            // Each lane evaluates one discharge population. Route its result to
            // arbitrary inherited muscles through the subgroup register crossbar.
            int signal=current_for((map>>(4*(tile&3)))&15,demand,direct,indirect,routing), minimum=signal;
            UNROLL
            for (Word d=1;d<4;d*=2) {
                int other=int(shuffle(Word(minimum),base+(tile^d),mask));
                if (other<minimum) minimum=other;
            }
            constexpr Word width=Tiles<8 ? Tiles : 8;
            UNROLL
            for (Word offset=0;offset<8;offset+=width) {
                Word m=(tile&(width-1))+offset,physical=effector(g,m);
                Word score=shuffle(Word(signal-minimum),base+channel(physical),mask);
                scores|=motor_current(physical,score,(basal>>(4*m))&15)<<(4*m);
            }
            UNROLL
            for (Word d=1;d<width;d*=2) scores|=shuffle(scores,base+(tile^d),mask);
        } else {
            int values[4],minimum=512;
            UNROLL
            for (Word a=0;a<4;++a) {
                values[a]=current_for((map>>(4*a))&15,demand,direct,indirect,routing);
                if (values[a]<minimum) minimum=values[a];
            }
            UNROLL
            for (Word m=0;m<8;++m) {
                Word physical=effector(g,m),code=channel(physical),score=0;
                UNROLL
                for (Word k=0;k<4;++k) if (code==k) score=Word(values[k]-minimum);
                scores|=motor_current(physical,score,(basal>>(4*m))&15)<<(4*m);
            }
        }
        return scores;
    }
};
