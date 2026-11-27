#pragma once
#include "report.cuh"

namespace axion::life::report_world {
struct Scene {
    Word places=0,recent=0,earlier=0;
    AX_FN void move(Word draw,Word delta) {
        Word s=draw&1,x=(places>>(2*s))&3,y=(x+delta)&3;
        places=(places&~(3u<<(2*s)))|(y<<(2*s));
        earlier=recent;recent=report::event(s,x,y);
    }
};
struct Lesson {
    Word n=0,source[report_length]{},base[report_length]{},offset=0,multiplier=1,optional=0;
    AX_FN Word say(const Scene &s,Word *words) const {
        Word length=n;
        if (optional && (s.recent&1)) length-=2;
        Episodes truth{0,s.recent,s.earlier};
        for (Word k=0;k<length;++k) {
            Word v=report::value(truth,source[k]);
            words[k]=1+((base[k]+v)*multiplier+offset)%report_words;
        }
        return length;
    }
};
AX_FN Lesson dialect(Word key,Word kind) {
    Lesson l{};Word age=kind==1 ? 3 : 0;
    l.n=kind>=2 ? 7 : 5;l.optional=kind==3;l.offset=key%report_words;
    Word strides[4]{1,5,7,11};l.multiplier=strides[(key>>8)&3];
    for (Word i=0;i<l.n;++i) {
        l.source[i]=i<3 ? 1+i+age : kind==2 && i<6 ? i+1 : 0;
        Word f=l.source[i];l.base[i]=f ? ((f==1 || f==4) ? 0 : 4) : 12+i;
    }
    // For short optional utterances every event component occurs before the tail.
    Word variable=kind==3 ? 5 : l.n;
    for (Word len=variable;len>1;--len) {
        key=mix(key+len);Word a=key%len,b=len-1;
        Word v=l.source[a];l.source[a]=l.source[b];l.source[b]=v;
        v=l.base[a];l.base[a]=l.base[b];l.base[b]=v;
    }
    return l;
}
struct Pupil { Episodes history{};VocalFrame construction{}; };
AX_FN void rear(Pupil &p,RecollectionGenes g,const Lesson &law,Word seed,Word lessons,Word reserved,Word cut) {
    Scene s{};s.places=seed&15;report::watch(p.history,g,report::sight(s.places,0));
    for (Word j=0;j<lessons;++j) {
        Word draw=mix(seed^j*0x55e14c1bu),delta=1+((draw>>8)%3);
        if (delta==reserved) delta=1+delta%3;
        s.move(draw,delta);report::watch(p.history,g,report::sight(s.places,0));
        if (draw&16) report::watch(p.history,g,report::sight(s.places,0));
        if (!s.earlier || cut==1 || !p.history.recent) continue;
        Scene teaching=s;
        if (cut==2) {teaching.recent^=24;teaching.earlier^=6;}
        Word words[report_length];Word n=law.say(teaching,words);
        report::model(p.construction,p.history,g,words,n);
    }
}
AX_FN RecallScore lifetime(RecollectionGenes g,Word key,Word cycles,Word lessons,Word setting,Word cut) {
    Word kind=setting==4 ? (mix(key+1)&3) : setting,reserved=1+mix(key+2)%3;
    auto law=dialect(mix(key+3),kind);Pupil writer{},reader{};
    rear(writer,g,law,mix(key+5),lessons,reserved,cut);rear(reader,g,law,mix(key+7),lessons,reserved,cut);
    Scene s{};s.places=key&15;report::watch(writer.history,g,report::sight(s.places,0));
    RecallScore totals{};
    for (Word trial=0;trial<cycles;++trial) {
        Word draw=mix(key^trial*0xb2571c49u);
        for (Word j=0;j<2;++j) {
            s.move(mix(draw+j),reserved);report::watch(writer.history,g,report::sight(s.places,0));
            if (draw&(8u<<j)) report::watch(writer.history,g,report::sight(s.places,0));
        }
        Episodes h=writer.history;
        if (cut==5) h.earlier=0;
        totals.faint+=h.recent==s.recent && h.earlier==s.earlier;
        Word truth[report_length]{},voiced[report_length]{},n=law.say(s,truth),credit=0;
        for (Word k=0;k<=n;++k) {
            auto &c=writer.construction.phases[k];Word v=report::fire(c,report::pathways(c,g),h,g);
            credit+=v==(k==n ? report_stop : truth[k]);
        }
        totals.one+=8*credit/(n+1);
        Word length=report::plan(writer.construction,h,g,voiced);bool exact=length==n;
        for (Word i=0;i<length && exact;++i) exact&=truth[i]==voiced[i];
        totals.delay+=exact;
        if (cut==3) length=0;
        if (cut==4) for (Word i=0;i<length/2;++i) {Word v=voiced[i];voiced[i]=voiced[length-i-1];voiced[length-i-1]=v;}
        Word belief[2]{};report::interpret(reader.construction,g,voiced,length,belief);
        Word current=1u<<(s.recent&31),older=1u<<(s.earlier&31);
        bool recovered=kind==1 ? belief[1]==older : kind==2 ? belief[0]==current && belief[1]==older : belief[0]==current;
        totals.noise+=recovered;
        Episodes partial=writer.history;partial.earlier=0;
        Word partial_length=report::plan(writer.construction,partial,g,voiced);
        bool transferable=partial_length==n;
        for (Word j=0;j<partial_length && transferable;++j) transferable&=voiced[j]==truth[j];
        totals.empty+=(kind==1 || kind==2) ? !partial_length : transferable;
        Episodes hidden=h;report::watch(hidden,g,0);
        totals.revision+=!report::plan(writer.construction,hidden,g,voiced);
    }
    return totals;
}
}
