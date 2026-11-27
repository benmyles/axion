#pragma once
#include "axion/composition.hpp"

namespace axion::life::composition {
AX_FN Word intern(Voices &v,Count pulse,RecollectionGenes g,bool recruit) {
    if (!pulse) return 0;
    Word index=mix(Word(pulse)^Word(pulse>>32))&255,cell=v.cache[index];
    Word best=65,margin=65;
    if (!cell || v.pulse[cell]!=pulse) {
        for (Word j=1;j<=v.n;++j) {
            Word d=Word(__builtin_popcountll(v.pulse[j]^pulse));
            if (d<best) {margin=best;best=d;cell=j;if (!best) break;}
            else if (d<margin) margin=d;
        }
        if (best>((g.low>>8)&15) || (best && margin==best)) {
            if (!recruit || v.n==forms) return 0;
            cell=++v.n;v.pulse[cell]=pulse;v.confidence[cell]=0;best=0;
        }
        v.cache[index]=static_cast<Byte>(cell);
    }
    if (recruit) {
        Count disagreement=v.pulse[cell]^pulse,confidence=v.confidence[cell];
        Count corrected=disagreement&((g.low&(1u<<12) ? ~confidence : 0ull)|(g.low&(1u<<13) ? confidence : 0ull));
        v.pulse[cell]^=corrected;
        Word rule=(g.low>>16)&15;
        v.confidence[cell]=((rule&1) ? ~(disagreement|confidence) : 0ull)|((rule&2) ? confidence&~disagreement : 0ull)
                          |((rule&4) ? disagreement&~confidence : 0ull)|((rule&8) ? disagreement&confidence : 0ull);
    }
    return cell;
}
AX_FN Phrase perceive(Voices &v,const Mouth &m,RecollectionGenes g,bool recruit) {
    Phrase p;
    for (Word i=0;i<m.size && i<width;++i) {
        Word id=intern(v,m.words[i],g,recruit);if (recruit && !id) return {};
        p.words[p.size++]=static_cast<Byte>(id);
    }
    return p;
}
AX_FN Mouth emit(const Voices &v,const Phrase &p) {
    Mouth out;out.size=p.size;
    for (Word i=0;i<p.size;++i) out.words[i]=v.pulse[p.words[i]];
    return out;
}
AX_FN bool equal(const Phrase &a,const Phrase &b) {
    if (a.size!=b.size) return false;
    for (Word k=0;k<a.size;++k) if (a.words[k]!=b.words[k]) return false;
    return true;
}
struct Context {Phrase cue{},prior{},spoken{};};
AX_FN Word select(const Context &c,Word pin) {
    if (pin>=3*width) return 0;
    const auto &p=pin<width ? c.cue : pin<2*width ? c.prior : c.spoken;
    Word i=pin%width;return i<p.size ? p.words[i] : 0;
}
struct Hit {Word value=0,time=0,reported=0;};
struct Gap {Word field=0,left=0,right=0,until=0,priority=0;};
struct Replay {
    const Detections &d;
    AX_FN Hit get(Word field,Word a,Word b,Word until) const {
        if (!field || !a) return {};
        Word key=address(field,a,b),i=d.tail[bucket(key)&511];
        while (i) {const auto &r=d.records[i-1];if (key_of(r.address)==key && r.time<=until) return {actual(r.address),r.time};i=r.previous;}
        return {};
    }
};
template<int Capacity> struct Awake {
    const Connections<Capacity> &m;
    AX_FN Hit get(Word field,Word a,Word b,Word until) const {
        if (!field || !a) return {};
        Word key=address(field,a,b),i=m.heads[bucket(key)&63];Hit best;
        while (i) {
            const auto &r=m.rows[i-1];
            if (key_of(r.address)==key && r.time<=until && r.time>best.time) best={actual(r.address),r.time,(r.address>>23)&1};
            i=m.chain[i-1];
        }
        return best;
    }
};
template<class Memory> AX_FN Word project(const Anatomy &a,const Memory &m,const Fibre &f,const Context &c,Word until,Word &depth,Gap *gap=nullptr,bool doubt=false) {
    if (f.literal) return f.literal;
    Word value=select(c,f.source);if (!value || !f.first) return value;
    if (f.gate) {
        auto status=m.get(f.gate,select(c,f.gate_source),0,until);
        if (status.value==f.condition) until=status.time;
    }
    Word other=select(c,f.other);
    for (Word j=0;j<2;++j) {
        Word id=j ? f.second : f.first;if (!id) break;
        auto &p=a.fields[id-1];
        if (p.right!=absent && !other) return 0;
        bool repeat=f.repeat && (j==1 || !f.second);
        Word visited[8]{},steps=0;
        for (;;) {
            Word mask=1u<<(value&31);
            if (repeat) {
                if (visited[value/32]&mask) return 0;
                visited[value/32]|=mask;
            }
            auto h=m.get(id,value,p.right==absent ? 0 : other,until);
            if (!h.value) {
                if (!steps) {if (gap) *gap={id,value,p.right==absent ? 0 : other,until};return 0;}
                break;
            }
            if (doubt && gap && h.reported && until>=h.time) {
                Word age=until-h.time+1;
                if (age>gap->priority) *gap={id,value,p.right==absent ? 0 : other,until,age};
            }
            value=h.value;++steps;++depth;
            if (!repeat) break;
        }
    }
    return value;
}
AX_FN Word fire(const Anatomy &p,const Phrase &cue) {
    for (Word i=0;i<p.phrases_n;++i) if (matches(p.phrases[i].cue,cue)) return i;
    return queries;
}
AX_FN Word stimulus(const Anatomy &p,const Phrase &line) {
    for (Word i=0;i<p.facts_n;++i) if (matches(p.facts[i],line)) return i;
    return patterns;
}
template<int Capacity> AX_FN void reindex(Connections<Capacity> &m) {
    for (auto &head:m.heads) head=0;
    for (Word i=0;i<m.n;++i) {
        Word h=bucket(key_of(m.rows[i].address))&63;
        m.chain[i]=m.heads[h];m.heads[h]=static_cast<Byte>(i+1);
    }
}
template<int Capacity> AX_FN bool protected_at(const Connections<Capacity> &m,const Anatomy &p,Word from,Word until) {
    if (!p.clocks) return false;
    for (Word j=0;j<m.n;++j) {
        const auto &r=m.rows[j];if (r.time<from || r.time>=until) continue;
        Word id=(r.address>>16)&255;if (!(p.clocks&(1ull<<(id-1)))) continue;
        if (Awake<Capacity>{m}.get(id,r.address&255,(r.address>>8)&255,~0u).time!=r.time) continue;
        for (Word c=0;c<p.phrases_n;++c) for (Word o=0;o<p.phrases[c].length;++o) {
            auto &f=p.phrases[c].voice[o];
            if (f.gate==id && f.condition==actual(r.address)) return true;
        }
    }
    return false;
}
template<int Capacity> AX_FN void prune(Connections<Capacity> &m,const Anatomy &p) {
    // Keep the current version and the version observed at each live cutoff.
    // Only the head of a clock address represents a live cutoff.
    Byte keep[Capacity];
    for (Word i=0;i<m.n;++i) {
        auto &r=m.rows[i];Word next=~0u;
        for (Word j=m.heads[bucket(key_of(r.address))&63];j;j=m.chain[j-1]) {
            auto &s=m.rows[j-1];if (s.time>r.time && s.time<next && key_of(s.address)==key_of(r.address)) next=s.time;
        }
        keep[i]=next==~0u || protected_at(m,p,r.time,next);
    }
    Word count=0;
    for (Word i=0;i<m.n;++i) if (keep[i]) m.rows[count++]=m.rows[i];
    m.n=count;reindex(m);
}
template<int Capacity> AX_FN void connect(Connections<Capacity> &m,const Anatomy &p,RecollectionGenes g,Word id,Word a,Word b,Word value,bool reported=false) {
    if (!a || !value || m.full) return;
    Word key=address(id,a,b),h=bucket(key)&63,latest=0;
    for (Word i=m.heads[h];i;i=m.chain[i-1]) if (key_of(m.rows[i-1].address)==key &&
        (!latest || m.rows[i-1].time>m.rows[latest-1].time)) latest=i;
    Word old=latest ? actual(m.rows[latest-1].address) : 0;
    Word payload=key|(value<<24)|(Word(reported)<<23);
    Word cue=Word(old!=0)+2*Word(old==value);
    if (old==value && latest && !reported) m.rows[latest-1].address&=~(1u<<23);
    if (!((g.low>>cue)&1) || old==value) return;
    bool dated=(p.dated&(1ull<<(id-1)))!=0;
    if (latest && (!dated || !protected_at(m,p,m.rows[latest-1].time,m.time))) {
        m.rows[latest-1]={payload,m.time};return;
    }
    if (m.n==Capacity) {prune(m,p);if (m.n==Capacity) {m.full=1;return;}}
    m.rows[m.n]={payload,m.time};m.chain[m.n]=m.heads[h];m.heads[h]=static_cast<Byte>(++m.n);
}
template<int Capacity> AX_FN void advance_time(Connections<Capacity> &m) {
    if (m.time==~0u) {
        Word rank[Capacity];
        for (Word i=0;i<m.n;++i) {rank[i]=1;for (Word j=0;j<m.n;++j) rank[i]+=m.rows[j].time<m.rows[i].time;}
        for (Word i=0;i<m.n;++i) m.rows[i].time=rank[i];
        m.time=m.n;
    }
    ++m.time;
}
template<int Capacity> AX_FN void impress(Connections<Capacity> &m,const Anatomy &p,RecollectionGenes g,Word type,const Phrase &x) {
    advance_time(m);
    auto used=p.used;
    while (used && !m.full) {
        Word i=__builtin_ctzll(used);used&=used-1;auto &field=p.fields[i];
        if (field.type==type) connect(m,p,g,i+1,x.words[field.left],field.right==absent ? 0 : x.words[field.right],x.words[field.out]);
    }
}
template<int Capacity> AX_FN Phrase act(const Anatomy &p,Connections<Capacity> &m,RecollectionGenes g,const Phrase &input,Word &handled,Word &depth,Gap *gap=nullptr,bool doubt=false) {
    Phrase reply;Word c=fire(p,input);handled=0;
    if (c<queries) {
        handled=2;const auto &q=p.phrases[c];
        if (q.ready && !m.full) {
            Context context{input,m.prior,m.spoken};
            for (Word i=0;i<q.length;++i) {
                Word y=project(p,Awake<Capacity>{m},q.voice[i],context,m.time,depth,gap,doubt);if (!y) return {};
                reply.words[reply.size++]=static_cast<Byte>(y);
            }
            m.prior=input;m.spoken=reply;
        }
    } else {
        Word t=stimulus(p,input);
        if (t<patterns) {impress(m,p,g,t,input);handled=1;}
    }
    return reply;
}
}
