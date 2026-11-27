#pragma once
#include "axion/sequence.hpp"
namespace axion::life::sequence {
struct PackedGraph {
    Word states=0,motors=0,senses=0,cell_bits=0,sense_bits=0,payload_words=0,known_words=0;
    AX_FN Word width() const {return cell_bits+sense_bits;}
    AX_FN Word words() const {return payload_words+known_words;}
};
AX_FN Word load_arc(PackedGraph g,const Word *pool,Word replica,Word stride,Word cell,Word motor) {
    Word slot=cell*g.motors+motor;
    if (g.known_words && !(pool[(g.payload_words+slot/32)*stride+replica]&(1u<<(slot%32)))) return 0;
    Word width=g.width(),offset=slot*width,shift=offset&31;
    Word packed=width ? pool[(offset/32)*stride+replica]>>shift : 0;
    if (shift+width>32) packed|=pool[(1+offset/32)*stride+replica]<<(32-shift);
    Word target=packed&((1u<<g.cell_bits)-1),sense=(packed>>g.cell_bits)&((1u<<g.sense_bits)-1);
    return contact(target,sense);
}
}
