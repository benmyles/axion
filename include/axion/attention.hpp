#pragma once
#include "model.hpp"

namespace axion::life {
// Two vocal pitches and two attention muscles. The conditional auditory
// contacts take the pitch the caller last emitted as proprioceptive input.
struct AttentiveEar {
    Word rule=0,previous=0,forms[8]{},contacts[4]{};
};
struct Iris { Word rule=0,contacts=0; };
static_assert(sizeof(AttentiveEar)==56 && sizeof(Iris)==8);
inline constexpr Word compiled_attention_bytes=20;
AX_FN Word auditory_charge(Word old,bool gain,Word threshold,Word rule) {
    Word condition=Word(old!=0)+2*Word(gain)+4*Word(old<threshold);
    int delta=int(((rule>>(4*condition))&15)^8)-8,next=int(old)+delta;
    return next<0 ? 0u : next>15 ? 15u : Word(next);
}
}
