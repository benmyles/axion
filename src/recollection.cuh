#pragma once
#include "axion/recollection.hpp"

namespace axion::life::engram {
AX_FN Word select_bits(Word selector,Word on,Word off) {
    return (selector&on)|(~selector&off);
}
AX_FN Word two_inputs(Word table,Word a,Word b) {
    Word low=select_bits(a,0u-((table>>1)&1),0u-(table&1));
    Word high=select_bits(a,0u-((table>>3)&1),0u-((table>>2)&1));
    return select_bits(b,high,low);
}
AX_FN Word contact_plane(Word table,Word low,Word high,Word cue,Word retina) {
    Word quiet=select_bits(cue,two_inputs(table>>4,low,high),two_inputs(table,low,high));
    Word active=select_bits(cue,two_inputs(table>>12,low,high),two_inputs(table>>8,low,high));
    return select_bits(retina,active,quiet)&0xffff;
}
AX_FN Word row(RecollectionGenes g,Word cue) {
    return (cue&(g.expression&3))^((g.expression>>2)&3);
}
AX_FN void see(Recollection &layer,Word cue,Word retinal_spikes,bool clear) {
    if (!retinal_spikes && !(layer.genes.expression&(1u<<9))) return;
    Word pre=15u<<(4*row(layer.genes,cue)),post=(retinal_spikes&15)*0x1111u;
    Word low=layer.weights&0xffff,high=layer.weights>>16;
    Word shift=16*Word(clear);
    layer.weights=contact_plane(layer.genes.low>>shift,low,high,pre,post)
                 |(contact_plane(layer.genes.high>>shift,low,high,pre,post)<<16);
}
AX_FN Word reinstate(const Recollection &layer,Word cue) {
    Word shift=4*row(layer.genes,cue),low=(layer.weights>>shift)&15,high=(layer.weights>>(shift+16))&15;
    Word threshold=1+((layer.genes.expression>>4)&3);
    Word winners=low&high;
    if (!winners && threshold<=2) winners=high;
    if (!winners && threshold==1) winners=low;
    if (threshold>3 || !winners) return 0;
    if (!(winners&(winners-1))) return winners;
    if (layer.genes.expression&(1u<<6)) return winners;
    return layer.genes.expression&(1u<<7) ? winners&-winners : 0u;
}
}
