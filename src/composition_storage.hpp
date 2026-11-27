#pragma once
#include "axion/composition.hpp"
#include <initializer_list>
#include <stdexcept>

namespace axion::life::composition {
inline void validate(const Mind &m) {
    const auto &a=m.anatomy;const auto &memory=m.memory;
    auto fail=[](){throw std::runtime_error("invalid compositional circuit");};
    if (a.facts_n>patterns || a.fields_n>projections || a.phrases_n>queries || m.lessons.n>episodes || m.lessons.n>m.lessons.seen || m.consolidated>m.lessons.seen ||
        m.lexicon.n>forms || memory.n>slots || memory.full>1 || (a.used>>a.fields_n) || ((a.clocks|a.dated)&~a.used)) fail();
    for (Word j=0;j<256;++j) if (m.lexicon.cache[j]>m.lexicon.n || (j && (j<=m.lexicon.n)!=bool(m.lexicon.pulse[j]))) fail();
    Word occupied=0;
    for (Word j=1;j<=forms;++j) occupied+=m.lexicon.pulse[j]!=0;
    if (occupied!=m.lexicon.n || m.lexicon.pulse[0]) fail();
    auto phrase=[&](const Phrase &x) {
        if (x.size>width) fail();
        for (Word i=0;i<x.size;++i) if (!x.words[i] || !m.lexicon.pulse[x.words[i]]) fail();
    };
    auto pattern=[&](const Pattern &p) {
        phrase(p.prototype);if (!p.fixed || (p.fixed>>p.prototype.size)) fail();
    };
    for (Word j=0;j<a.facts_n;++j) pattern(a.facts[j]);
    for (Word j=0;j<a.fields_n;++j) {
        const auto &p=a.fields[j];if (p.type>=a.facts_n) fail();Word n=a.facts[p.type].prototype.size;
        if (p.left>=n || p.out>=n || (p.right!=absent && p.right>=n)) fail();
    }
    for (Word j=0;j<a.phrases_n;++j) {
        const auto &q=a.phrases[j];pattern(q.cue);if (!q.length || q.length>width || q.ready>1) fail();
        for (Word i=0;i<q.length;++i) {
            const auto &f=q.voice[i];
            for (Word pin:{f.source,f.other,f.gate_source}) if (pin!=absent && pin>=3*width) fail();
            if (f.first>a.fields_n || f.second>a.fields_n || f.gate>a.fields_n || f.repeat>1 ||
                (f.literal && !m.lexicon.pulse[f.literal]) || (f.condition && !m.lexicon.pulse[f.condition])) fail();
        }
    }
    for (Word j=0;j<m.lessons.n;++j) {phrase(m.lessons.event[j].stimulus);phrase(m.lessons.event[j].response);}
    phrase(memory.prior);phrase(memory.spoken);
    const auto &ask=m.pending;
    if (ask.field) {
        phrase(ask.original);phrase(ask.prompt);
        if (ask.field>a.fields_n || !ask.left || ask.construction>=a.phrases_n || ask.channel>=a.phrases[ask.construction].length ||
            ask.exchanges<1 || ask.exchanges>3 || !matches(a.phrases[ask.construction].cue,ask.prompt)) fail();
        for (Word i=0;i<a.phrases[ask.construction].length;++i) if (i!=ask.channel) {
            auto &f=a.phrases[ask.construction].voice[i];if (!f.literal && (f.first || f.source>=ask.prompt.size)) fail();
        }
    }
    bool seen[slots]{};
    for (Word k=0;k<64;++k) {
        Word i=memory.heads[k];
        while (i) {
            if (i>memory.n || seen[i-1]) fail();
            seen[i-1]=true;const auto &r=memory.rows[i-1];Word key=key_of(r.address),field=key>>16;
            if (!field || field>a.fields_n || (bucket(key)&63)!=k || !r.time || r.time>memory.time || !m.lexicon.pulse[actual(r.address)] ||
                !m.lexicon.pulse[key&255] || ((key>>8)&255 && !m.lexicon.pulse[(key>>8)&255])) fail();
            i=memory.chain[i-1];
        }
    }
    for (Word j=0;j<memory.n;++j) if (!seen[j]) fail();
}
}
