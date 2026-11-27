#pragma once
#include "procedure_ear.cuh"

namespace axion::life::procedure {
struct Hearing {
    Count first=0,previous=0,low=0,high=0;
    Word frames=0,crests=0,active=0,overflow=0;
};
AX_FN Count envelope_noise(Word &seed) {seed=mix(seed+0x9e3779b9u);return (Count(mix(seed^0x27d4eb2du))<<32)|seed;}
template<class Receiver> AX_FN void release(Hearing &h,RecollectionGenes g,Receiver &receiver) {
    if (!h.active) return;
    receiver((g.high&128) && h.crests>1 ? h.high : h.first);h.active=0;h.crests=0;
}
template<class Receiver> AX_FN void sample(Hearing &h,Count frame,RecollectionGenes g,Receiver &receiver) {
    ++h.frames;Word energy=__builtin_popcountll(frame),on=14+((g.expression>>19)&7),off=5+((g.expression>>24)&7);
    if (h.active && (energy<=off || ((g.high&64) && energy>=on && __builtin_popcountll(h.previous^frame)>22))) release(h,g,receiver);
    if (energy<on) return;
    if (!h.active) {h.active=1;h.first=frame;h.low=frame;h.high=frame;}
    else {
        // Two saturating confidence bits for each of the 64 receptors.
        Count top=h.high&h.low,bottom=~(h.high|h.low);
        Count next_low=(frame&(~h.low|top))|(~frame&~h.low&~bottom);
        Count next_high=(frame&(h.high|h.low))|(~frame&top);h.low=next_low;h.high=next_high;
    }
    h.previous=frame;++h.crests;
}
template<class Out> AX_FN void phonation(Count pulse,Word &key,Word rate,Word noise,bool legato,Out &out) {
    Word ticks=1+(mix(key+1543)%rate);
    if (!legato) {Count a=envelope_noise(key),b=envelope_noise(key),c=envelope_noise(key);out(pulse&a&b&c);}
    for (Word j=0;j<ticks;++j) {
        Count wave=pulse;for (Word k=0;k<noise;++k) {key=mix(key+0x64c3a51bu);wave^=1ull<<(key&63);}
        out(wave);
    }
    if (!legato) {Count a=envelope_noise(key),b=envelope_noise(key),c=envelope_noise(key),d=envelope_noise(key);out(pulse&a&b&c&d);}
}
}
