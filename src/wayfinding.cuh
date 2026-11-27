// Each row is an ensemble with four efference-tagged contacts. The address
// bits and the three sensory coincidence bits obey the same inherited table.
enum class MapCut : Word { intact, silent, fixed, one_pulse, erased, no_appetite, no_soma, no_dendrites, isolated, disinhibited, raw, trace_fixed, immediate, no_hold, labile, no_novelty, fixed_support };
#include "temporal.cuh"
AX_FN MapGenes map_founder(Word seed) {
    return {0xaaaaaaaau,(mix(seed^0x8f325cd5u)&~63u)|8u,mix(seed^0x33f28bc1u),mix(seed^0xd63e7f15u)};
}
AX_FN MapGenes inherit_map(MapGenes p,const Genome &g,const OutcomeGenes &sensory,Word key,bool mutate,Word &edits) {
    if (!mutate) return p;
    Word chance=4+((g.reproduction>>8)&31);
    Word words[]{p.plastic,p.wiring,p.spread,p.currents};
    UNROLL
    for (Word k=0;k<4;++k) {
        Word noise=mix(key^(0x745a8bebu*(k+7)));
        if ((noise&255)<chance) { words[k]^=1u<<((noise>>8)&31);++edits; }
    }
    Word draw=mix(key^0x5a19384du);
    if ((draw&511)<chance) {
        Word shift=(draw>>10)&16;
        Word donor=(draw&(1u<<17)) ? sensory.plastic>>(16-shift) : g.expression>>16;
        Word next=(words[0]&~(65535u<<shift))|((donor&65535)<<shift);
        edits+=next!=words[0];words[0]=next;
    }
    return {words[0],words[1],words[2],words[3]};
}
AX_FN Word basin_cell(Word pos,Settings c,Word &orientation) {
    Word x=pos&(c.side-1),y=pos/c.side;
    orientation=0;
    if (c.basin_width) {
        Word width=c.basin_width,shift=Word(__builtin_ctz(width)),sector=(x>>shift)+(y>>shift)*(c.side>>shift);
        orientation=mix(c.seed^sector*0xbd29a79du^0x77d4899bu)&3;
        Word u=x&(width-1),v=y&(width-1);
        x=(orientation&1) ? v : u;y=(orientation&1) ? u : v;
        if (orientation==1 || orientation==2) x=width-1-x;
        if (orientation>=2) y=width-1-y;
    }
    return (x&7)|((y&3)<<3);
}
AX_FN Word landmark(Word pos,Settings c) {
    Word frame,place=basin_cell(pos,c,frame);
    if (c.basin_width && place>=28) place=28+frame;
    return 1u<<((place*((c.seed>>8)|1)+c.seed)&31);
}
AX_FN Word aquifer(Word pos,Word phase,Settings c) {
    Word frame,site=basin_cell(pos,c,frame);
    Word k=mix(c.seed^0xcdabe397u);
    site=(site*((k>>5)|1)+k)&31;site^=site>>3;site=(site*((k>>14)|1)+(k>>20))&31;
    if (site&7) return 0;
    return 1+(((site>>3)+((k>>29)|1)*phase)&3);
}

struct MapOutput {
    Word currents=0,shift=0;
    bool shunt=false;
    AX_FN Word apply(Word basal,MotorCounts *votes) const {
        if (shunt && votes) { votes->low=0;votes->high=0; }
        // Saturate eight independent four-bit currents. The lower three
        // bits sum without a carry into the next muscle's register.
        Word b=(basal>>shift)&((15u>>shift)*0x11111111u);
        Word lower=(currents&0x77777777u)+(b&0x77777777u);
        Word high=(currents^b)&0x88888888u;
        Word carry=((currents&b)|(lower&(currents|b)))&0x88888888u;
        return (lower^high)|((carry>>3)*15u);
    }
};
template<int Tiles> struct MapCircuit : Temporal<Tiles,temporal_image> {
    static constexpr int rows=32/Tiles;
    MapGenes gene;
    static constexpr int value_words=Tiles<4 ? 4/Tiles : 1;
    Word contacts[rows],attractors[value_words],dirty=0;
    MapCut cut=MapCut::intact;
    AX_FN void load_trace(const OutcomeLayer *organs,Word sites,Word slot,Word tile,bool fresh,Word mode) {
        if constexpr (temporal_image) if (mode>=4) this->attach(trace_tail(organs,sites)[slot],tile,fresh,mode);
    }
    AX_FN void store_trace(OutcomeLayer *organs,Word sites,Word slot,Word tile) const {
        if constexpr (temporal_image) if (this->attached) this->save(trace_tail(organs,sites)[slot],tile);
    }
    AX_FN void frozen_graph(Word tile,Word base,Word mask) {
        if constexpr (temporal_image) if (this->attached) {
            if (cut!=MapCut::raw && cut!=MapCut::silent)
                this->project(contacts,tile,base,mask,~0u,cut==MapCut::immediate,cut==MapCut::no_hold,cut==MapCut::no_novelty);
            this->attached=false;
        }
    }
    AX_FN void sense_event(Word raw,Word tile,Word base,Word mask) {
        if constexpr (temporal_image) if (this->attached && raw) this->hear(address(raw),tile,base,mask);
    }
    AX_FN Word prior() const { return 0u-((gene.wiring>>31)&1); }
    AX_FN void load(const MapLayer &m,Word tile,bool fresh) {
        gene=m.genes;
        UNROLL
        for (int j=0;j<rows;++j) contacts[j]=fresh ? prior() : m.contacts[tile+Tiles*j];
        UNROLL
        for (int j=0;j<value_words;++j) attractors[j]=fresh ? 0u-((gene.wiring>>30)&1) : m.appetites[(tile+j*Tiles)&3];
        dirty=fresh ? ~0u : 0;
    }
    AX_FN void store(MapLayer &m,Word tile) const {
        UNROLL
        for (int j=0;j<rows;++j) if (dirty&(1u<<j)) m.contacts[tile+Tiles*j]=contacts[j];
        UNROLL
        for (int j=0;j<value_words;++j) if (tile+j*Tiles<4 && (dirty&(1u<<(30+j)))) m.appetites[tile+j*Tiles]=attractors[j];
    }
    AX_FN Word address(Word spikes) const {
        Word sensory=rotate(spikes,(gene.wiring>>9)&31);
        return sensory ? Word(__builtin_ctz(sensory)) : 0;
    }
    AX_FN Word channel(Word pins) const {
        return ((pins>>(gene.wiring&7))&1)|(((pins>>((gene.wiring>>3)&7))&1)<<1);
    }
    AX_FN Word note(Word raw,Word pins) const { return raw ? 0x2000u|(pins<<5)|address(raw) : 0; }
    AX_FN void associate(Word site,Word food,Word tile) {
        if (!food || cut==MapCut::fixed || cut==MapCut::no_appetite) return;
        UNROLL
        for (int j=0;j<value_words;++j) {
            Word sense=1u<<site,table=(gene.plastic>>16)>>(4*Word(((tile+j*Tiles)&3)+1==food));
            Word next=lut2(table,attractors[j],sense);
            dirty|=Word(next!=attractors[j])<<(30+j);attractors[j]=next;
        }
    }
    AX_FN void encounter(Word previous,Word destination,Word food,Word tile) {
        if (!previous || !destination || cut==MapCut::fixed) return;
        Word old=previous&31,ports=channel(previous>>5),target=address(destination);
        if constexpr (temporal_image) if (this->attached && cut!=MapCut::trace_fixed) {
            Word lane=index()&31,base=lane&~(Tiles-1),mask=Word((1ull<<Tiles)-1)<<base,bank=0;
            UNROLL
            for (int j=0;j<rows;++j) if (old/Tiles==Word(j)) bank=contacts[j];
            bank=shuffle(bank,base+(old&(Tiles-1)),mask);
            this->credit(old,ports,target,(known(bank)>>ports)&1,(bank>>(8*ports))&31,tile,cut==MapCut::labile,cut==MapCut::no_novelty,cut==MapCut::fixed_support);
        }
        Word observed=target|32u|(old==target ? 128u : 64u);
        Word pattern=observed*0x01010101u,effector=255u<<(8*ports);
        UNROLL
        for (int j=0;j<rows;++j) {
            bool active=tile+Tiles*j==old;
            Word before=contacts[j],table=gene.plastic>>(active ? 8 : 0);
            Word next=mux(effector,lut2(table>>4,before,pattern),lut2(table,before,pattern));
            dirty|=Word(next!=before)<<j;contacts[j]=next;
        }
        associate(old,food,tile);
    }
    AX_FN Word known(Word bank) const {
        Word flags=(bank>>(5+(gene.wiring>>20)%3))&0x01010101u;
        return ((flags*0x01020408u)>>24)&15;
    }
    AX_FN Word conduct(Word bank,Word frontier,Word connected) const {
        Word signals=0;
        UNROLL
        for (Word a=0;a<4;++a) signals|=((frontier>>((bank>>(8*a))&31))&1)<<a;
        return signals&connected;
    }
    AX_FN int excitation(Word a,Word here,Word origin,Word connected,Word response,Word start,Word table,Word mode,Word pacemaker) const {
        Word index=((connected>>a)&1)+2*((response>>a)&1);
        int current=int((table>>(4*index))&15);
        if (mode>=2) {
            bool moved=((here>>(8*a))&31)!=origin && (connected&(1u<<a));
            int motion=int(((gene.spread>>(start ? 28 : 20))&15)^8)-8;
            if (moved) current+=motion;
            if (((gene.wiring>>(14+Word(start!=0)))&1) && a==(pacemaker&3)) current+=(gene.spread>>24)&15;
        }
        return current;
    }
    AX_FN Word muscle(Word physical,Word value) const {
        if ((gene.wiring&(1u<<19)) && !(physical&(1u<<((gene.wiring>>16)&7)))) return 0;
        return lesser(15,value);
    }
    AX_FN MapOutput output(const Genome &g,Word raw,Word goal,Word needs,Word tile,Word base,Word mask,
                           Word mode,Word pacemaker) const {
        if (cut==MapCut::silent || !raw) return {};
        Word origin=address(raw),start=goal ? rotate(goal,(gene.wiring>>9)&31) : 0;
        if (needs && cut!=MapCut::no_appetite) {
            Word stored=0;
            UNROLL
            for (int j=0;j<value_words;++j) if (needs&(1u<<((tile+j*Tiles)&3))) stored|=attractors[j];
            start|=ensemble_or<Tiles>(stored,tile,base,mask);
        }
        Word view[rows];
        UNROLL
        for (int j=0;j<rows;++j) view[j]=contacts[j];
        if constexpr (temporal_image) if (this->attached && cut!=MapCut::raw)
            this->project(view,tile,base,mask,start ? ~0u : 1u<<origin,cut==MapCut::immediate,cut==MapCut::no_hold,cut==MapCut::no_novelty);
        Word local=0;
        UNROLL
        for (int j=0;j<rows;++j) if (origin/Tiles==Word(j)) local=view[j];
        Word here=shuffle(local,base+(origin&(Tiles-1)),mask);
        Word connected=known(here),available[rows];
        UNROLL
        for (int j=0;j<rows;++j) available[j]=known(view[j]);
        Word frontier=start,response=0;
        Word duration=cut==MapCut::one_pulse ? 1 : 1+((gene.spread>>8)&15);
        for (Word pulse=0;pulse<duration && frontier;++pulse) {
            Word hit=conduct(here,frontier,connected);
            response=lut2(gene.spread>>16,response,hit)&15;
            if (hit && (gene.spread&(1u<<12))) break;
            Word next=0;
            UNROLL
            for (int j=0;j<rows;++j) {
                Word cell=tile+j*Tiles,bit=1u<<cell;
                Word excitation=conduct(view[j],frontier,available[j]);
                Word input=Word(bool(frontier&bit))+2*Word(excitation!=0)+4*Word(bool(start&bit));
                next|=((gene.spread>>input)&1)<<cell;
            }
            next=ensemble_or<Tiles>(next,tile,base,mask);
            if (next==frontier) {
                Word remaining=duration-pulse-1;
                if (remaining && !(gene.spread&(1u<<13))) {
                    // With constant presynaptic input each readout bit follows
                    // a unary Boolean map. Two steps cover its longest cycle.
                    response=lut2(gene.spread>>16,response,hit)&15;
                    if (!(remaining&1)) response=lut2(gene.spread>>16,response,hit)&15;
                }
                break;
            }
            frontier=next;
        }
        Word table=gene.currents>>(start ? 16 : 0),scores=0;
        bool amplify=gene.wiring&(1u<<28),center=amplify || mode>=2;
        Word strength=amplify ? 1+((gene.wiring>>(start ? 26 : 24))&3) : 1;
        Word condition=2*Word(start!=0)+Word(start ? response && response!=15 : connected!=15);
        bool regulated=mode>=3 && cut!=MapCut::disinhibited;
        bool quiet=cut==MapCut::no_dendrites || cut==MapCut::isolated || (regulated && ((gene.plastic>>(24+condition))&1));
        bool shunt=cut==MapCut::no_soma || cut==MapCut::isolated || (regulated && ((gene.plastic>>(28+condition))&1));
        Word leak=(gene.wiring>>6)&3;
        if constexpr (Tiles>=4) {
            int current=excitation(tile&3,here,origin,connected,response,start,table,mode,pacemaker),minimum=0;
            if (center) {
                minimum=current;
                UNROLL
                for (Word d=1;d<4;d*=2) {
                    int other=int(shuffle(Word(minimum),base+(tile^d),mask));
                    if (other<minimum) minimum=other;
                }
            }
            constexpr Word width=Tiles<8 ? Tiles : 8;
            UNROLL
            for (Word n=0;n<8;n+=width) {
                Word m=n+(tile&(width-1)),pins=effector(g,m);
                Word signal=shuffle(Word(current-minimum)*strength,base+channel(pins),mask);
                scores|=muscle(pins,signal)<<(4*m);
            }
            UNROLL
            for (Word d=1;d<width;d*=2) scores|=shuffle(scores,base+(tile^d),mask);
        } else {
            int levels[4],minimum=center ? 64 : 0;
            UNROLL
            for (Word a=0;a<4;++a) {
                levels[a]=excitation(a,here,origin,connected,response,start,table,mode,pacemaker);
                if (center && levels[a]<minimum) minimum=levels[a];
            }
            UNROLL
            for (Word m=0;m<8;++m) {
                Word pins=effector(g,m),code=channel(pins),signal=0;
                UNROLL
                for (Word a=0;a<4;++a) if (a==code) signal=Word(levels[a]-minimum)*strength;
                scores|=muscle(pins,signal)<<(4*m);
            }
        }
        return {scores,quiet || leak==3 ? 4 : leak,shunt};
    }
    AX_FN Word drive(const Genome &g,Word raw,Word goal,Word needs,Word basal,Word tile,Word base,Word mask,
                     Word mode=1,Word pacemaker=0,MotorCounts *votes=nullptr) const {
        return output(g,raw,goal,needs,tile,base,mask,mode,pacemaker).apply(basal,votes);
    }
};
