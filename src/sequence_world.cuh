#pragma once
#include "sequence.cuh"

namespace axion::life::chambers {
enum Family : Word { ring,latches,network,hideout,rhythm };
AX_FN Count pattern(Word key) {return Count(mix(key))|(Count(mix(key^0xb7e15163u))<<32);}
AX_FN Chamber grow(Word family,Word seed,Word n,Word motors,Word colours) {
    Chamber w{};w.states=n;w.motors=motors;w.senses=colours;
    for (Word a=0;a<motors;++a) w.gestures[a]=pattern(seed^a*0x45d9f3bu);
    for (Word j=0;j<colours;++j) w.sounds[j]=pattern(seed^0x3b5ae4d7u^j*0xe2d3a017u);
    for (Word q=0;q<n;++q) for (Word a=0;a<motors;++a) {
        Word key=mix(seed^q*0xb5297a4du^a*0x8f75628du),to=q,out=0;
        if (family==ring) {to=(q+(a&1 ? n-1 : 1))%n;out=to==0 ? 1 : 0;}
        else if (family==latches) {if (a+1<motors) to=(q^(1u<<(a%5)))%n;else out=(q^(q>>1))%colours;}
        else if (family==rhythm) {to=(q+1)%n;out=(mix(seed+q)&3)%colours;}
        else {to=a==0 ? (q+1)%n : key%n;out=mix(key+23)%colours;}
        w.rule[q][a]=contact(to,out);
    }
    return w;
}
// The observer remembers the object's side only while present. Speech is a
// delayed physical output of this caregiver. The student sees none of these
// variables, and its developmental genes are selected in other environments.
AX_FN Chamber den(const Chamber &channels,Word seed) {
    Chamber w=channels;w.states=32;w.origin=4;
    Word reverse=(seed>>8)&1;
    for (Word q=0;q<32;++q) for (Word a=0;a<w.motors;++a) {
        Word mode=q>>3,side=q&1,seen=(q>>1)&1,present=(q>>2)&1,out=0;
        if (a==1 || a==2) {side=(a-1)^reverse;if (present) seen=side;}
        if (a==3) present=0;
        if (a==4) {present=1;seen=side;}
        if (a==5) mode=1;
        if (mode==1 && (a==6 || a==7)) {mode=a==6 ? 2 : 3;out=a==6 ? 1 : 2;}
        if (!a && mode>=2) {out=3+((mode==2 ? side : seen)^reverse);mode=0;}
        w.rule[q][a]=contact((mode<<3)|(present<<2)|(seen<<1)|side,out);
    }
    return w;
}
AX_FN Count experience(const Chamber &w,Word &state,Word action,Word &weather,Word noise) {
    Word arc=w.rule[state][action];state=cell_of(arc);Count pulse=w.sounds[sense_of(arc)];
    for (Word bit=0;bit<noise;++bit) {weather=mix(weather+0x9e3779b9u);pulse^=1ull<<(weather&63);}
    return pulse;
}
}
