#pragma once
#include "composition.cuh"

namespace axion::life::composition {
constexpr Word catalog_size=256;
struct Stimulus {Phrase words{};Byte type=absent,query=absent;};
struct Catalog {
    Stimulus turns[catalog_size]{};
    Word n=0;
};
struct Atlas {
    Word keys[slots]{};
    Byte hash[512]{},encode[projections][256]{},decode[projections][256]{},values[projections]{},widths[projections]{};
    Word close[projections][8]{};Count dated=0;
    unsigned short offset[slots]{};
    Byte aperture[slots]{};
    Word n=0,views=0,data_words=0,view_words=0,unsupported=0;
    AX_FN Word words() const {return data_words+views*view_words;}
    AX_FN Word slot(Word key) const {
        Word h=bucket(key)&511;
        for (;;) {Word i=hash[h];if (!i || keys[i-1]==key) return i;h=(h+1)&511;}
    }
};
AX_FN void admit(Atlas &t,Word field,Word a,Word b,Word value) {
    Word key=address(field,a,b),h=bucket(key)&511;
    while (t.hash[h] && t.keys[t.hash[h]-1]!=key) h=(h+1)&511;
    if (!t.hash[h]) {
        if (t.n==slots) {t.unsupported=1;return;}
        t.keys[t.n]=key;t.hash[h]=static_cast<Byte>(++t.n);
    }
    Word i=field-1;
    if (!t.encode[i][value]) {Word j=++t.values[i];t.encode[i][value]=static_cast<Byte>(j);t.decode[i][j]=static_cast<Byte>(value);}
}
AX_FN void pack_layout(Atlas &t,const Anatomy &p,const Catalog &c) {
    t.dated=p.dated;
    for (Word k=0;k<c.n;++k) {
        auto &x=c.turns[k];Count used=p.used;
        while (used) {
            Word j=__builtin_ctzll(used);used&=used-1;const auto &f=p.fields[j];
            if (x.type==f.type) admit(t,j+1,x.words.words[f.left],f.right==absent ? 0 : x.words.words[f.right],x.words.words[f.out]);
        }
    }
    for (Word j=0;j<p.phrases_n;++j) if (p.phrases[j].ready) for (Word o=0;o<p.phrases[j].length;++o) {
        auto &f=p.phrases[j].voice[o];if (f.gate) t.close[f.gate-1][f.condition/32]|=1u<<(f.condition&31);
    }
    Word bits=0;
    for (Word v=0;v<2;++v) {
        for (Word i=0;i<t.n;++i) {
            Word field=(t.keys[i]>>16)-1;bool dated=(p.dated>>field)&1;
            if (dated==(v!=0)) continue;
            Word width=32-Word(__builtin_clz(Word(t.values[field])));
            t.widths[field]=static_cast<Byte>(width);t.offset[i]=static_cast<unsigned short>(bits);bits+=width;
            if ((p.clocks>>field)&1) t.aperture[i]=static_cast<Byte>(++t.views);
        }
        if (!v) t.view_words=(bits+31)/32;
    }
    t.data_words=(bits+31)/32;
}
struct Packed {
    const Atlas &t;
    Word *pool;
    Word self,stride;
    AX_FN Word read(Word base,Word start,Word width) const {
        Word word=base+start/32,shift=start&31;Word value=pool[word*stride+self]>>shift;
        if (shift+width>32) value|=pool[(word+1)*stride+self]<<(32-shift);
        return value&((1u<<width)-1);
    }
    AX_FN void write(Word start,Word width,Word value) {
        Word word=start/32,shift=start&31,mask=(1u<<width)-1;
        Word &low=pool[word*stride+self];low=(low&~(mask<<shift))|(value<<shift);
        if (shift+width>32) {
            Word &high=pool[(word+1)*stride+self];high=(high&~(mask>>(32-shift)))|(value>>(32-shift));
        }
    }
    AX_FN Hit get(Word field,Word a,Word b,Word until) const {
        if (!field || !a) return {};
        if (until!=~0u && !(t.dated&(1ull<<(field-1)))) return {};
        Word i=t.slot(address(field,a,b));if (!i) return {};
        if (until!=~0u && (!until || until>t.views)) return {};
        Word base=until==~0u ? 0 : t.data_words+(until-1)*t.view_words;
        Word code=read(base,t.offset[i-1],t.widths[field-1]);return {t.decode[field-1][code],t.aperture[i-1],0};
    }
    AX_FN void impress(const Anatomy &p,RecollectionGenes g,const Stimulus &s) {
        Count used=p.used,capture=0;
        while (used) {
            Word j=__builtin_ctzll(used);used&=used-1;auto f=p.fields[j];if (s.type!=f.type) continue;
            Word a=s.words.words[f.left],b=f.right==absent ? 0 : s.words.words[f.right],y=s.words.words[f.out];
            Word i=t.slot(address(j+1,a,b));if (!i) continue;
            Word code=t.encode[j][y],old=read(0,t.offset[i-1],t.widths[j]);
            Word cue=Word(old!=0)+2*Word(old==code);
            if (!code || old==code || !((g.low>>cue)&1)) continue;
            write(t.offset[i-1],t.widths[j],code);
            if (t.close[j][y/32]&(1u<<(y&31))) capture|=1ull<<j;
        }
        // All contacts of the event are live before a new view is captured.
        while (capture) {
            Word j=__builtin_ctzll(capture);capture&=capture-1;auto f=p.fields[j];
            Word i=t.slot(address(j+1,s.words.words[f.left],f.right==absent ? 0 : s.words.words[f.right]));
            for (Word k=0;k<t.view_words;++k) pool[(t.data_words+(t.aperture[i-1]-1)*t.view_words+k)*stride+self]=pool[k*stride+self];
        }
    }
};
struct Discourse {Phrase prior{},reply{};};
AX_FN Phrase spoken(const Anatomy &p,Packed m,Discourse &history,RecollectionGenes g,const Stimulus &s) {
    Phrase answer;
    if (s.query!=absent) {
        const auto &q=p.phrases[s.query];if (!q.ready) return {};
        Context c{s.words,history.prior,history.reply};
        for (Word j=0;j<q.length;++j) {
            Word depth=0,y=project(p,m,q.voice[j],c,~0u,depth);if (!y) return {};
            answer.words[answer.size++]=static_cast<Byte>(y);
        }
        history={s.words,answer};
    } else if (s.type!=absent) m.impress(p,g,s);
    return answer;
}
AX_FN Word select_turn(Word k,Word &random,Word entries) {
    random=1664525u*random+1013904223u;
    return k<16 ? (k*13)%entries : Word((Count(random)*entries)>>32);
}
AX_FN Word digest(Word hash,const Phrase &p) {
    hash=(hash^p.size)*0x01000193u;
    for (Word i=0;i<p.size;++i) hash=(hash^p.words[i])*0x01000193u;
    return hash;
}
}
