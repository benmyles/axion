#pragma once
#include "report.cuh"
#include "axion/commons.hpp"

namespace axion::life::culture {
struct Passage { Word positions[3]{},events[2]{}; };
AX_FN Passage motion(Word draw,Word reserved,bool lesson) {
    Passage p{};p.positions[0]=mix(draw)&15;
    for (Word j=0;j<2;++j) {
        Word key=mix(draw^(j+1)*0x8b2d3907u),s=key&1,delta=reserved;
        if (lesson) {delta=1+((key>>5)%3);if (delta==reserved) delta=1+delta%3;}
        Word from=(p.positions[j]>>(2*s))&3,to=(from+delta)&3;
        p.positions[j+1]=(p.positions[j]&~(3u<<(2*s)))|(to<<(2*s));p.events[j]=report::event(s,from,to);
    }
    return p;
}
AX_FN Word wind(Word seed,Word i) {return 1+mix(seed^(i/32)*0x4bd81123u)%3;}
AX_FN Passage day(Word seed,Word i,Word t,Word season) {
    return motion(mix(seed^i*0xbb1800b1u^t*0x8b9d731du),1+(wind(seed,i)-1+season)%3,false);
}
AX_FN void witness(Episodes &h,RecollectionGenes g,const Passage &p) {
    for (Word seen:p.positions) report::watch(h,g,report::sight(seen,0));
}
AX_FN Word target_age(const CommonsSettings &c,Word i,Word t) {return c.event_age<2 ? c.event_age : (mix(c.seed^(i/2)^t)&1);}
// These unused portions of the heritable speech program wire the untrained
// articulator. A pulse may project a tonic cell, either event, or a repeated
// feature. No ordering or acoustic meaning is assigned by the environment.
AX_FN Word tract(RecollectionGenes g) {return mix((g.low>>24)^(g.high&0xfffff000u)^(g.expression>>16));}
AX_FN Count babbled_sound(Word key,Word cell) {
    constexpr Word onset[8]{2,4,7,11,12,13,14,16},vowel[4]{1,5,9,15},coda[4]{12,13,14,18};
    Word address=(cell*((key>>8)|1)+key)&31;
    return onset[address&7]|(vowel[address>>3]<<5)|(coda[(key>>12)&3]<<10);
}
AX_FN Utterance babble(const Episodes &h,Word cue,RecollectionGenes g) {
    Utterance out{};Word key=mix(tract(g)^cue);out.count=2+(key%7);out.handled=1;
    for (Word k=0;k<out.count;++k) {
        Word wiring=mix(key^k*0xd8eb3e25u),source=wiring%report_sources,v=report::value(h,source);
        if (v>=4) {out.count=0;break;}
        Word cell=((wiring>>8)+v*((wiring>>16)|1))%20;
        out.sound[k]=babbled_sound(key,cell);
    }
    return out;
}
AX_FN Utterance express(const Reports &r,const Episodes &h,Word cue,bool invent,Word &improvised) {
    auto emitted=report::articulate(r,h,report::frame(r,cue,false));
    if (invent && !emitted.count) {emitted=babble(h,cue,r.genes);improvised+=bool(emitted.count);}
    return emitted;
}
AX_FN Word warp_sum(Word value) {
    for (Word stride=16;stride;stride>>=1) {Word other;asm("shfl.sync.bfly.b32 %0,%1,%2,31,0xffffffff;":"=r"(other):"r"(value),"r"(stride));value+=other;}
    return value;
}
AX_FN Word neighbor(Word v,Word displacement) {
    Word out;asm("shfl.sync.bfly.b32 %0,%1,%2,31,0xffffffff;":"=r"(out):"r"(v),"r"(displacement));return out;
}
AX_FN Utterance transmit(const Utterance &sent,Word cut) {
    Utterance out{};Word across=cut==3 ? 3 : 1;
    out.count=neighbor(sent.count,across);
    for (Word k=0;k<report_length;++k) {
        Count sound=sent.sound[k];
        Word low=neighbor(Word(sound),across),high=neighbor(Word(sound>>32),across);
        out.sound[k]=(Count(high)<<32)|low;
    }
    if (cut==1) out.count=0;
    if (cut==2) for (Word k=0;k<out.count/2;++k) {
        Count a=out.sound[k];out.sound[k]=out.sound[out.count-1-k];out.sound[out.count-1-k]=a;
    }
    return out;
}
}
ENTRY void culture_origin(const CommonsSettings *settings,CulturalIsland *islands,const Reports *pupils,const Reports *founder) {
    Word island=index();auto c=*settings;if (island>=c.count/32) return;
    auto &t=islands[island];t={};
    if (!c.culture) t.teacher=founder[c.alternating ? island&1 : 0];
    else t.teacher.genes=pupils[32*island+(mix(c.seed^island)&31)].genes;
}
namespace axion::life::culture {
AX_FN void learn_from_tutor(const CommonsSettings &c,CulturalIsland &patch,Reports &learner,Word i) {
    const auto &tutor=patch.teacher;
    Word accepted=0,improvised=0;Episodes demonstration{};
    for (Word k=0;k<c.tutoring && c.culture!=2;++k) {
        auto scene=culture::motion(mix(c.seed^i*0x6b251793u^k*0xd61c19abu),culture::wind(c.seed,i),true);
        culture::witness(demonstration,tutor.genes,scene);culture::witness(learner.history,learner.genes,scene);
        Word age=c.event_age==2 ? k&1 : c.event_age,cue=c.cues[age];
        auto sound=culture::express(tutor,demonstration,cue,c.culture==1,improvised);
        accepted+=report::learn(learner,cue,sound);
    }
    if (!c.report_cache) report::invalidate(learner);
    accepted=culture::warp_sum(accepted);improvised=culture::warp_sum(improvised);
    if (!(i&31)) {patch.accepted=accepted;patch.improvised=improvised;patch.words=patch.investigated=0;}
}
}
ENTRY void culture_rear(const CommonsSettings *settings,CulturalIsland *islands,Reports *pupils,Word first,Word batch) {
    Word j=index();if (j>=batch) return;Word i=first+j;
    culture::learn_from_tutor(*settings,islands[i/32],pupils[i],i);
}
ENTRY void culture_rear_local(const CommonsSettings *settings,CulturalIsland *islands,Reports *pupils,Word first,Word batch) {
    Word j=index();if (j>=batch) return;Word i=first+j;
    Reports own=pupils[i];
    culture::learn_from_tutor(*settings,islands[i/32],own,i);
    pupils[i]=own;
}
ENTRY void culture_exchange(const CommonsSettings *settings,Forager *animals,Reports *reports,const Bindings *bindings,
                            const Response *muscles,Encounter *encounters,AcousticState *social,CulturalIsland *islands,const Reservoir *water) {
    Word i=index();auto c=*settings;if (i>=c.count) return;
    auto &a=animals[i];auto &r=reports[i];auto &call=social[i];
    Word age=culture::target_age(c,i,a.day),cue=c.cues[age],birth_noise=0;
    Word season=water[i/32].phase;auto own=culture::day(c.seed,i,a.day,season);
    if (a.energy) culture::witness(r.history,r.genes,own);
    Utterance sent{};if (a.energy && c.soma) sent=culture::express(r,r.history,cue,c.culture==1,birth_noise);
    Utterance received=culture::transmit(sent,c.report_wire);call.calls+=bool(sent.count);
    call.heard=sent.count;call.heard_calls+=a.energy && received.count;
    Word field=report::frame(r,cue,false);r.request=field<2 ? field+1 : 0;
    if (a.energy) report::receive(r,received);
    Word belief=a.energy ? r.heard[age] : 0;
    call.decoded=belief && !(belief&(belief-1)) ? belief : 0;
    Word address=Word(!call.decoded)+2*Word(a.receipt==2)+4*((muscles[i].ports>>((bindings[i].genes.competition>>24)&7))&1);
    bool investigate=a.energy && (c.policy==2 || (!c.policy && ((bindings[i].genes.competition>>(8+address))&1)));
    auto other=culture::day(c.seed,i^1,a.day,season);
    Word actual=other.events[1-age]&31;
    call.readings+=a.energy && call.decoded;
    call.accurate+=a.energy && call.decoded==(1u<<actual);
    call.prior_call=Word(investigate);
    Word updated=0;
    if (investigate && c.ear_learning) {
        Episodes visited{};culture::witness(visited,r.genes,other);
        updated=report::learn_view(r,visited,cue,received);
        if (!c.report_cache) report::invalidate(r);
    }
    Word heard=call.decoded ? Word(__builtin_ctz(call.decoded)) : 0;
    Word external=call.decoded ? (1u<<(heard&1))|(4u<<((heard>>1)&3))|(64u<<(heard>>3)) : 0;
    encounters[i]={external,0x01070000u,0,0,a.receipt==1 ? 1u : a.receipt==2 ? 2u : 0u,a.energy ? EventKind::step : EventKind::probe};
    Word words=culture::warp_sum(sent.count),acquired=culture::warp_sum(updated),novel=culture::warp_sum(birth_noise);
    if (!(i&31)) {auto &t=islands[i/32];t.words+=words;t.investigated+=acquired;t.improvised+=novel;}
}
ENTRY void culture_handoff(const CommonsSettings *settings,CulturalIsland *islands,const Reports *adults,const Forager *animals,
                           CulturalSample *samples,Word renew) {
    Word k=index();auto c=*settings;if (k>=c.count/32) return;
    Word mass=0;for (Word j=0;j<32;++j) mass+=animals[32*k+j].offspring;
    Word chosen=mix(c.seed^k*0x611a742bu)&31;
    if (mass && c.teachers!=2) {
        Word draw=mix(c.seed^k*0xffa84265u)%mass;chosen=0;
        while (chosen<31 && draw>=animals[32*k+chosen].offspring) {draw-=animals[32*k+chosen].offspring;++chosen;}
    }
    auto &island=islands[k];const auto &adult=adults[32*k+chosen];
    CulturalSample sample{};sample.accepted=island.accepted;sample.words=island.words;
    sample.improvised=island.improvised;sample.investigated=island.investigated;
    for (Count word:adult.syllables) sample.forms+=bool(word);
    Episodes seen{};Word positions[3]{0,2,6};for (Word position:positions) report::watch(seen,adult.genes,report::sight(position,0));
    Word ignored=0;sample.voice=culture::express(adult,seen,c.cues[c.event_age==1],c.culture==1,ignored);
    samples[k]=sample;
    if (renew && !c.teachers) {
        if (mass) island.teacher=adult;else island.teacher={};
    } else if (renew && c.teachers==2) island.teacher=adult;
}
