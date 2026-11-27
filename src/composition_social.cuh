#pragma once
#include "composition.cuh"

namespace axion::life::composition {
// Recover an acoustic question by running a single learned projection in
// reverse. Its variable ports have to bind all the missing contact keys.
AX_FN Question formulate(const Anatomy &p,const Gap &gap,const Phrase &original,Word exchanges) {
    for (Word j=0;j<p.phrases_n;++j) {
        const auto &q=p.phrases[j];if (!q.ready) continue;
        for (Word k=0;k<q.length;++k) {
            const auto &f=q.voice[k];
            if (f.first!=gap.field || f.second || f.repeat || f.gate || f.source>=q.cue.prototype.size) continue;
            Phrase prompt=q.cue.prototype;Word filled=q.cue.fixed;
            if (filled&(1u<<f.source)) continue;
            prompt.words[f.source]=static_cast<Byte>(gap.left);filled|=1u<<f.source;
            if (gap.right) {
                if (f.other>=prompt.size || (filled&(1u<<f.other))) continue;
                prompt.words[f.other]=static_cast<Byte>(gap.right);filled|=1u<<f.other;
            }
            if (filled+1!=(1u<<prompt.size) || equal(prompt,original)) continue;
            bool interpretable=true;
            for (Word n=0;n<q.length;++n) if (n!=k) {
                auto w=q.voice[n];interpretable&=w.literal || (!w.first && w.source<prompt.size);
            }
            if (interpretable) return {original,prompt,static_cast<Byte>(gap.field),static_cast<Byte>(gap.left),
                static_cast<Byte>(gap.right),static_cast<Byte>(j),static_cast<Byte>(k),static_cast<Byte>(exchanges+1)};
        }
    }
    return {};
}
AX_FN Word interpret(const Anatomy &p,const Question &ask,const Phrase &heard) {
    auto &q=p.phrases[ask.construction];if (heard.size!=q.length) return 0;
    for (Word n=0;n<q.length;++n) if (n!=ask.channel) {
        auto f=q.voice[n];
        Word expected=f.literal ? f.literal : ask.prompt.words[f.source];
        if (expected!=heard.words[n]) return 0;
    }
    return heard.words[ask.channel];
}
AX_FN void converse(Mind &m,Exchange &p,bool social) {
    auto preview=perceive(m.lexicon,p.heard,m.genes,false);p.depth=0;p.status=0;p.reply={};
    if (!m.pending.field && fire(m.anatomy,preview)==queries && stimulus(m.anatomy,preview)==patterns) return;
    auto input=perceive(m.lexicon,p.heard,m.genes,true);
    if (input.size!=p.heard.size) {p.status=3;return;}
    Word exchanges=0;
    if (m.pending.field) {
        auto question=m.pending;m.pending={};
        Word value=interpret(m.anatomy,question,input);
        if (value) {
            advance_time(m.memory);connect(m.memory,m.anatomy,m.genes,question.field,question.left,question.right,value,true);
            input=question.original;exchanges=question.exchanges;
        }
    }
    Gap gap;auto reply=act(m.anatomy,m.memory,m.genes,input,p.status,p.depth,&gap,p.evidence!=0);
    if ((!reply.size || p.evidence) && gap.field && social && gap.until==m.memory.time && exchanges<3 && (m.genes.high&256)) {
        m.pending=formulate(m.anatomy,gap,input,exchanges);
        if (m.pending.field) {p.reply=emit(m.lexicon,m.pending.prompt);p.status=4;++m.turns;return;}
    }
    p.reply=emit(m.lexicon,reply);++m.turns;
}
}

namespace axion::life::composition {
template<class Memory> struct Counterfactual {
    const Memory &base;const Anatomy &anatomy;const Phrase &line;Word type;RecollectionGenes genes;Word time;
    AX_FN Hit get(Word id,Word a,Word b,Word until) const {
        auto old=base.get(id,a,b,until);
        if (time>until || !id) return old;
        const auto &p=anatomy.fields[id-1];
        if (p.type!=type || a!=line.words[p.left] || b!=(p.right==absent ? 0u : Word(line.words[p.right]))) return old;
        Word value=line.words[p.out],cue=Word(old.value!=0)+2*Word(old.value==value);
        if (((genes.low>>cue)&1) && old.value!=value) return {value,time,0};
        return old;
    }
};
// Invert the acquired speech circuit with an isolated one-contact overlay.
// A proposed motor has to produce the target under the queried objective.
AX_FN Phrase synthesise(const Mind &m,const Phrase &objective,const Phrase &wanted,Word &trials) {
    auto &p=m.anatomy;Word q=fire(p,objective);if (q==queries || !p.phrases[q].ready || !wanted.size || m.memory.time==~0u) return {};
    auto &request=p.phrases[q];if (request.length!=wanted.size) return {};
    Word now=m.memory.time;
    Context context{objective,m.memory.prior,m.memory.spoken};Awake<slots> base{m.memory};
    Word depth=0;Phrase current;
    for (Word k=0;k<request.length;++k) {
        Word v=project(p,base,request.voice[k],context,now,depth);if (!v) break;
        current.words[current.size++]=static_cast<Byte>(v);
    }
    if (equal(current,wanted)) return {};
    for (Word o=0;o<request.length;++o) {
        auto f=request.voice[o];if (!f.first || f.literal) continue;
        Word source=select(context,f.source),other=select(context,f.other);if (!source) continue;
        // Follow prefixes that the current objective already binds. Each
        // visited address is a possible site for a genuinely causal write.
        Word trail[2]{f.first,f.second};Word index=f.second ? 1 : 0;
        for (Word hop=0;hop<=index;++hop) {
            Word id=trail[hop];auto field=p.fields[id-1];Word b=field.right==absent ? 0 : other;
            if (field.right!=absent && !b) break;
            for (Word j=0;j<m.lexicon.n;++j) {
                Word y=j+1;
                auto rule=p.facts[field.type];Phrase move=rule.prototype;Word bound=rule.fixed;
                move.words[field.left]=static_cast<Byte>(source);bound|=1u<<field.left;
                if (field.right!=absent) {move.words[field.right]=static_cast<Byte>(b);bound|=1u<<field.right;}
                move.words[field.out]=static_cast<Byte>(y);bound|=1u<<field.out;
                if (bound+1!=(1u<<move.size)) continue;
                Counterfactual<Awake<slots>> possible{base,p,move,field.type,m.genes,now+1};
                ++trials;Phrase hypothetical;
                for (Word k=0;k<request.length;++k) {
                    Word next=project(p,possible,request.voice[k],context,now+1,depth);
                    if (!next || k>=wanted.size || next!=wanted.words[k]) break;
                    hypothetical.words[hypothetical.size++]=static_cast<Byte>(next);
                }
                if (equal(hypothetical,wanted)) return move;
            }
            auto v=base.get(id,source,b,now);if (!v.value) break;source=v.value;
        }
    }
    return {};
}
}
