#pragma once
#include "composition.cuh"

namespace axion::life::composition {
AX_FN bool same_shape(const Pattern &a,const Pattern &b) {
    if (a.prototype.size!=b.prototype.size || a.fixed!=b.fixed) return false;
    for (Word k=0;k<a.prototype.size;++k) if ((a.fixed&(1u<<k)) && a.prototype.words[k]!=b.prototype.words[k]) return false;
    return true;
}
AX_FN void wiring(Anatomy &a) {
    a.used=a.dated=a.clocks=0;
    for (Word i=0;i<a.phrases_n;++i) if (a.phrases[i].ready) for (Word k=0;k<a.phrases[i].length;++k) {
        const auto &f=a.phrases[i].voice[k];Count fields=0;
        if (f.first) fields|=1ull<<(f.first-1);
        if (f.second) fields|=1ull<<(f.second-1);
        if (f.gate) {Count bit=1ull<<(f.gate-1);a.clocks|=bit;a.used|=bit;a.dated|=fields;}
        a.used|=fields;
    }
}
AX_FN Word merge(Anatomy &dst,const Anatomy &src) {
    Byte map[projections+1]{};Count used=src.used;
    while (used) {
        Word j=__builtin_ctzll(used);used&=used-1;auto field=src.fields[j];
        auto &shape=src.facts[field.type];Word type=0;
        while (type<dst.facts_n && !same_shape(dst.facts[type],shape)) ++type;
        if (type==patterns) return 1;
        if (type==dst.facts_n) dst.facts[dst.facts_n++]=shape;
        field.type=static_cast<Byte>(type);Word k=0;
        while (k<dst.fields_n) {
            auto &p=dst.fields[k];if (p.type==field.type && p.left==field.left && p.right==field.right && p.out==field.out) break;
            ++k;
        }
        if (k==projections) return 1;
        if (k==dst.fields_n) dst.fields[dst.fields_n++]=field;
        map[j+1]=static_cast<Byte>(k+1);
    }
    for (Word i=0;i<src.phrases_n;++i) {
        auto q=src.phrases[i];if (!q.ready) continue;
        Word j=0;while (j<dst.phrases_n && !same_shape(dst.phrases[j].cue,q.cue)) ++j;
        if (j==queries) return 1;
        for (Word k=0;k<q.length;++k) {
            auto &f=q.voice[k];f.first=map[f.first];f.second=map[f.second];f.gate=map[f.gate];
        }
        if (j==dst.phrases_n) ++dst.phrases_n;
        dst.phrases[j]=q;
    }
    wiring(dst);return 0;
}
}
