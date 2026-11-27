#pragma once
#include "procedure_policy.cuh"

namespace axion::life::procedure {
// Only the used prefix crosses into the inference buffer. Counts and
// destinations have independent widths; a constant field occupies no bits.
struct CompactTransfer {
    Count floor=0;Word low_key=0,high_key=0;
    Byte inputs=0,low_bits=0,base=0,destination_bits=0,count_bits=0,cut=0,n=0,reserved=0;
    Count bits[71]{};
};
AX_FN Word width(Count n) {return n ? 64-Word(__builtin_clzll(n)) : 0;}
AX_FN Word squeezed(Word value,Word mask) {
    Word result=0,at=0;
    for (Word k=0;k<6;++k) if (mask&(1u<<k)) result|=((value>>k)&1)<<at++;
    return result;
}
AX_FN void write_bits(Count *out,Word index,Word depth,Count value) {
    if (!depth) return;
    Word bit=index*depth,unit=bit/64,offset=bit&63;
    out[unit]|=value<<offset;if (offset+depth>64) out[unit+1]|=value>>(64-offset);
}
AX_FN Count read_bits(const Count *out,Word index,Word depth) {
    if (!depth) return 0;
    Word bit=index*depth,unit=bit/64,offset=bit&63;Count x=out[unit]>>offset;
    if (offset+depth>64) x|=out[unit+1]<<(64-offset);
    return depth==64 ? x : x&((1ull<<depth)-1);
}
AX_FN Word compact(const Transfer &source,CompactTransfer &v) {
    v={};Word active=0,small=64,large=0;Count lo=~0ull,hi=0;
    for (Word i=0;i<64;++i) {
        Word dst=source.destination[i]<64 ? source.destination[i] : 64;Count n=dst==64 ? 0 : source.pulses[i];
        if (n<lo) lo=n;
        if (n>hi) hi=n;
        if (dst<small) small=dst;
        if (dst>large) large=dst;
        for (Word k=1;k<=32;k*=2) if (!(i&k)) {
            Word j=i|k;bool value=source.destination[i]!=source.destination[j];
            active|=(value || (dst!=64 && n!=source.pulses[j])) ? k : 0;
        }
    }
    Word count=1u<<__builtin_popcount(active);v.inputs=static_cast<Byte>(active);v.base=static_cast<Byte>(small);
    v.floor=lo;v.destination_bits=static_cast<Byte>(width(large-small));v.count_bits=static_cast<Byte>(width(hi-lo));
    v.low_bits=static_cast<Byte>(__builtin_popcount(active&7));
    for (Word i=0;i<8;++i) {v.low_key|=squeezed(i,active&7)<<(3*i);v.high_key|=squeezed(i,active>>3)<<(3*i);}
    v.cut=static_cast<Byte>((count*v.destination_bits+63)/64);
    v.n=static_cast<Byte>(v.cut+(count*v.count_bits+63)/64);
    for (Word i=0;i<64;++i) if (!(i&~active)) {
        Word dst=source.destination[i]<64 ? source.destination[i] : 64;Count n=dst==64 ? 0 : source.pulses[i];Word index=squeezed(i,active);
        write_bits(v.bits,index,v.destination_bits,dst-small);write_bits(v.bits+v.cut,index,v.count_bits,n-lo);
    }
    return __builtin_offsetof(CompactTransfer,bits)+sizeof(Count)*v.n;
}
AX_FN Landing evaluate(const CompactTransfer &p,Word state) {
    Word index=state;
    if (p.inputs!=63) index=((p.low_key>>(3*(state&7)))&7)|(((p.high_key>>(3*(state>>3)))&7)<<p.low_bits);
    Word end=p.base+Word(read_bits(p.bits,index,p.destination_bits));
    if (end>=64) return {};
    return {p.floor+read_bits(p.bits+p.cut,index,p.count_bits),end};
}
}
