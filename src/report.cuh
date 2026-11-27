#pragma once
#include "axion/report.hpp"

namespace axion::life::report {
AX_FN Word event(Word channel,Word from,Word to) {return 32u|channel|(from<<1)|(to<<3);}
AX_FN Word sight(Word scene,Word shutters) {
    return ((shutters&1) ? 0 : 1u<<(scene&3)) | ((shutters&2) ? 0 : 16u<<(scene>>2));
}
AX_FN void watch(Episodes &h,RecollectionGenes g,Word retina) {
    Word old=h.retina;h.retina=retina;
    Word record=0,kind=3;
    if ((old&15) && (old>>4) && (retina&15) && (retina>>4)) {
        Word diff=old^retina;bool a=diff&15,b=diff>>4;
        kind=Word(a)+Word(b);
        if (kind==1) {
            Word side=Word(b);
            record=event(side,Word(__builtin_ctz((old>>(side*4))&15)),Word(__builtin_ctz((retina>>(side*4))&15)));
        }
    }
    Word gate=(g.expression>>(8+2*kind))&3;
    if (gate==1) {h.earlier=h.recent;h.recent=record;}
    if (gate==2) {h.earlier=0;h.recent=0;}
    if (gate==3) h.recent=record;
}
AX_FN Word value(const Episodes &h,Word source) {
    if (!source) return 0;
    Word e=source<4 ? h.recent : h.earlier;
    if (!(e&32)) return 4;
    Word limb=source<4 ? source-1 : source-4;
    return limb==0 ? (e&1) : limb==1 ? ((e>>1)&3) : ((e>>3)&3);
}
AX_FN Word association(Word contact,Word feature) {return feature<4 ? (contact>>(5*feature))&31 : 0;}
// Four 5-bit motor addresses and a four-bit strength fit in one word.
// An inherited table controls writes; a second controls confidence changes.
AX_FN void pair(Word &contact,Word feature,Word heard,RecollectionGenes g) {
    if (feature>=4) return;
    Word old=association(contact,feature),strength=(contact>>20)&15;
    Word signal=!old ? 0 : old==heard ? 1 : 2,index=4*signal+(strength>>2);
    if ((g.high>>index)&1)
        contact=(contact&~(31u<<(5*feature)))|(heard<<(5*feature));
    Word step=(g.low>>(2*index))&3;
    if (step==1 && strength<15) ++strength;
    if (step==2 && strength) --strength;
    if (step==3) strength=0;
    contact=(contact&0xfffffu)|(strength<<20);
}
AX_FN Word token(Reports &r,Count spelling,bool acquire) {
    if (!spelling) return 0;
    Word free=report_words;
    for (Word k=0;k<report_words;++k) {
        if (r.syllables[k]==spelling) return k+1;
        if (!r.syllables[k] && free==report_words) free=k;
    }
    if (acquire && free<report_words) {r.syllables[free]=spelling;return free+1;}
    return 0;
}
AX_FN Word frame(const Reports &r,Word cue,bool acquire) {
    for (Word j=0;j<2;++j) if (r.frames[j].cue==cue && cue) return j;
    if (acquire && cue) for (Word j=0;j<2;++j) if (!r.frames[j].cue) return j;
    return 2;
}
AX_FN Word recruit(const Reports &r,Word cue) {
    Word k=frame(r,cue,true),policy=(r.genes.expression>>16)&3;
    if (k<2 || !policy || !cue) return k;
    if (policy==2) return mix(cue)&1;
    if (policy==3 && r.request) return 2-r.request;
    Word strength[2]{};
    for (Word f=0;f<2;++f) for (const auto &c:r.frames[f].phases) {
        Word peak=0;for (Word afferent:c.contacts) {Word p=(afferent>>20)&15;if (p>peak) peak=p;}
        strength[f]+=peak;
    }
    return strength[0]==strength[1] ? mix(cue)&1 : Word(strength[1]<strength[0]);
}
AX_FN Word vocabulary(const VocalFrame &f) {
    Word used=0;
    for (const auto &c:f.phases) for (Word contact:c.contacts) for (Word v=0;v<4;++v) {
        Word id=association(contact,v);
        if (id && id<=report_words) used|=1u<<(id-1);
    }
    return used;
}
AX_FN void hear_prompt(Reports &r,Word form,bool first) {
    if (first) r.request=0;
    Word k=frame(r,form,false);if (k<2) r.request=k+1;
}
AX_FN Word direct_pathways(const VocalColumn &c,RecollectionGenes g);
AX_FN void model(VocalFrame &f,const Episodes &h,RecollectionGenes g,const Word *heard,Word n) {
    for (Word phase=0;phase<=n;++phase) {
        auto &column=f.phases[phase];
        for (Word j=0;j<report_sources;++j) pair(column.contacts[j],value(h,j),phase<n ? heard[phase] : report_stop,g);
        column.contacts[0]=(column.contacts[0]&0x00ffffffu)|((128u|direct_pathways(column,g))<<24);
    }
}
AX_FN bool learn_view(Reports &r,const Episodes &view,Word cue,const Utterance &demonstration) {
    if (!demonstration.count || demonstration.count>report_length || !view.recent) return false;
    Word k=recruit(r,cue);
    if (k>=2) return false;
    // Recruit the whole utterance or leave the dictionary and contacts intact.
    Word needed=0,empty=0,retain=0,heard[report_length];bool replacing=r.frames[k].cue && r.frames[k].cue!=cue;
    for (Word i=0;i<demonstration.count;++i) {
        if (!demonstration.sound[i]) return false;
        Word id=token(r,demonstration.sound[i],false);heard[i]=id;bool fresh=!id;
        if (id) retain|=1u<<(id-1);
        for (Word j=0;j<i;++j) fresh&=demonstration.sound[i]!=demonstration.sound[j];
        needed+=fresh;
    }
    if (needed) for (Count s:r.syllables) empty+=s==0;
    if (needed>empty) {
        retain|=vocabulary(r.frames[k^1]);if (!replacing) retain|=vocabulary(r.frames[k]);
        Word available=((1u<<report_words)-1)&~retain;
        if (Word(__builtin_popcount(available))<needed) return false;
        while (available) {Word j=Word(__builtin_ctz(available));available&=available-1;r.syllables[j]=0;}
    }
    if (replacing) {r.frames[k]={};if (r.request==k+1) r.request=0;}
    if (needed) for (Word j=0;j<demonstration.count;++j) if (!heard[j]) heard[j]=token(r,demonstration.sound[j],true);
    r.frames[k].cue=cue;model(r.frames[k],view,r.genes,heard,demonstration.count);return true;
}
AX_FN bool learn(Reports &r,Word cue,const Utterance &demonstration) {return learn_view(r,r.history,cue,demonstration);}
AX_FN Word direct_pathways(const VocalColumn &c,RecollectionGenes g) {
    Word best=0,selected=0,slack=(g.expression>>4)&3,threshold=g.expression&15;
    for (Word contact:c.contacts) {Word p=(contact>>20)&15;if (p>best) best=p;}
    if (best<threshold) return 0;
    for (Word j=0;j<report_sources;++j) {
        Word contact=c.contacts[j],p=(contact>>20)&15;
        if ((contact&0xfffffu) && p>=threshold && p+slack>=best) selected|=1u<<j;
    }
    if ((g.expression&64) && (selected&1)) return 1;
    return selected;
}
AX_FN Word pathways(const VocalColumn &c,RecollectionGenes g) {
    Word header=c.contacts[0]>>24;
    return (header&128) ? (header&127) : direct_pathways(c,g);
}
AX_FN void invalidate(Reports &r) {
    for (auto &f:r.frames) for (auto &c:f.phases) c.contacts[0]&=0x00ffffffu;
}
AX_FN Word fire(const VocalColumn &column,Word active,const Episodes &h,RecollectionGenes g) {
    if (!active) return 0;
    Word motors=~0u;
    while (active) {
        Word j=Word(__builtin_ctz(active));active&=active-1;
        Word context=value(h,j),contact=column.contacts[j],v=association(contact,context);
        if (context==4 && (g.expression&128)) {
            // A missing afferent can still have a determined output if every
            // one of its possible activations drives the same acoustic cell.
            v=association(contact,0);Word n=(j==1 || j==4) ? 2 : 4;
            for (Word b=1;b<n;++b) if (association(contact,b)!=v) return 0;
        }
        if (!v) return 0;
        motors&=1u<<(v-1);
    }
    return motors ? Word(__builtin_ctz(motors))+1 : 0;
}
AX_FN Word plan(const VocalFrame &f,const Episodes &h,RecollectionGenes g,Word *motors) {
    for (Word phase=0;phase<=report_length;++phase) {
        auto &c=f.phases[phase];Word spike=fire(c,pathways(c,g),h,g);
        if (spike==report_stop) return phase;
        if (!spike || spike>report_words || phase==report_length) return 0;
        motors[phase]=spike;
    }
    return 0;
}
AX_FN Utterance articulate(const Reports &r,const Episodes &h,Word k) {
    Utterance out{};if (k>=2) return out;
    out.handled=1;Word motors[report_length];
    out.count=plan(r.frames[k],h,r.genes,motors);
    for (Word i=0;i<out.count;++i) out.sound[i]=r.syllables[motors[i]-1];
    return out;
}
AX_FN Utterance speak(const Reports &r) {return articulate(r,r.history,r.request ? r.request-1 : 2);}
AX_FN Word possibilities(Word channels,Word sources,Word destinations) {
    Word wave=0;
    for (Word to=0;to<4;++to) if (destinations&(1u<<to)) for (Word from=0;from<4;++from)
        if (to!=from && (sources&(1u<<from))) wave|=(channels&3)<<((from<<1)|(to<<3));
    return wave;
}
// Sound constrains the listener's own feature cells. Each bit in the resulting
// wave denotes a physical movement still consistent with every heard pulse.
AX_FN void interpret(const VocalFrame &f,RecollectionGenes g,const Word *heard,Word n,Word *belief) {
    Word allowed[report_sources]={1,3,15,15,3,15,15};belief[0]=belief[1]=0;
    if (!n || n>report_length) return;
    for (Word phase=0;phase<=n;++phase) {
        const auto &c=f.phases[phase];Word active=pathways(c,g),target=phase<n ? heard[phase] : report_stop;
        if (!active || !target) return;
        while (active) {
            Word j=Word(__builtin_ctz(active));active&=active-1;Word matches=0;
            for (Word v=0;v<4;++v) if (association(c.contacts[j],v)==target) matches|=1u<<v;
            allowed[j]&=matches;if (!allowed[j]) return;
        }
    }
    Word a=possibilities(allowed[1],allowed[2],allowed[3]),b=possibilities(allowed[4],allowed[5],allowed[6]);
    if (a && b) {belief[0]=a;belief[1]=b;}
}
AX_FN void receive(Reports &r,const Utterance &utterance) {
    r.heard[0]=r.heard[1]=0;r.heard_phases=0;
    if (!r.request || r.request>2 || !utterance.count || utterance.count>report_length) return;
    Word heard[report_length];
    for (Word i=0;i<utterance.count;++i) {
        heard[i]=token(r,utterance.sound[i],false);if (!heard[i]) return;
    }
    interpret(r.frames[r.request-1],r.genes,heard,utterance.count,r.heard);r.heard_phases=utterance.count;
}
AX_FN void listen(Reports &r,Word cue,const Utterance &u) {
    if (cue) {Word k=frame(r,cue,false);r.request=k<2 ? k+1 : 0;}
    if (r.request || cue) {receive(r,u);return;}
    Word union_a=0,union_b=0,phases=0;
    for (Word k=0;k<2;++k) if (r.frames[k].cue) {
        r.request=k+1;receive(r,u);union_a|=r.heard[0];union_b|=r.heard[1];
        if (r.heard[0]) phases=r.heard_phases;
    }
    r.request=0;r.heard[0]=union_a;r.heard[1]=union_b;r.heard_phases=phases;
}
AX_FN Word values(Word events,Word part) {
    if (!part) return Word(bool(events&0x55555555u))|(Word(bool(events&0xaaaaaaaau))<<1);
    Word options=0;
    for (Word v=0;v<4;++v) {
        Word mask=part==1 ? 0x03030303u<<(2*v) : 255u<<(8*v);
        options|=Word(bool(mask&events))<<v;
    }
    return options;
}
AX_FN Utterance recount(const Reports &r,Word cue) {
    Utterance out{};Word k=frame(r,cue,false);if (k>=2 || !r.heard[0] || !r.heard[1]) return out;
    out.handled=1;Word possible[report_sources]{1};
    for (Word t=0;t<2;++t) for (Word v=0;v<3;++v) possible[1+3*t+v]=values(r.heard[t],v);
    for (Word step=0;step<=report_length;++step) {
        const auto &c=r.frames[k].phases[step];Word active=pathways(c,r.genes),motor=0;
        while (active) {
            Word j=Word(__builtin_ctz(active));active&=active-1;Word options=possible[j];
            while (options) {
                Word v=Word(__builtin_ctz(options));options&=options-1;
                Word pulse=association(c.contacts[j],v);
                if (!pulse || (motor && motor!=pulse)) return out;
                motor=pulse;
            }
        }
        if (motor==report_stop) {out.count=step;return out;}
        if (!motor || motor>report_words || step==report_length) return out;
        out.sound[step]=r.syllables[motor-1];
    }
    return out;
}
}
