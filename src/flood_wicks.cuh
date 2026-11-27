#pragma once
#include "wick_physics.cuh"
namespace axion::life::flood {
AX_FN void count(Count &total, Word amount);
AX_FN void tag_strand(World w, Word side, Word a, Word b, Word before, Word after) {
    auto f=w.wicks;
    if ((!w.drift_marks.items && (!f.parcels || bool(before)==bool(after))) || before==after)
        return;
    Word x = a % side, u = b % side, axis = x == u, owner;
    if (axis)
        owner = ((a / side + 1) % side) == b / side ? a : b;
    else
        owner = ((x + 1) % side) == u ? a : b;
    drift::disturb(w.drift_marks, side, owner, axis);
    if(!f.parcels || bool(before)==bool(after))return;
    if(f.reeds.seams)runnel::set_fold(f.reeds.seams[owner],axis,0);
    auto &tag = f.parcels[owner];
    Word bit = wicks::edge_bit(axis);
    if (after) {
        tag |= bit;
        count(f.ledger->strung, 1);
    } else {
        tag &= ~bit;
        count(f.ledger->frayed, 1);
    }
}
AX_FN void capillary(World w, Configuration c, Word index, Word a, Word b, Word axis, Silt &left, Silt &right,
                     Word fibre, Word tick) {
    auto f = w.wicks;
    if (!f.parcels || !(f.mode & wicks::flowing) || !fibre)
        return;
    bool outer = practice::tile_at(a, c.side) != practice::tile_at(b, c.side);
    if (outer && !(f.mode & wicks::across))
        return;
    auto &p = w.hollows.beds[a], &q = w.hollows.beds[b];
    auto reed=f.reeds;
    auto ga=w.matter.bed[a],gb=w.matter.bed[b];
    Word mask=3,fold=reed.seams?runnel::fold_at(reed.seams[a],axis):0;
    int boost=0,base=int(left.water*Word(2+material::weight(gb)+(q.relief&7)))-int(right.water*Word(2+material::weight(ga)+(p.relief&7)));
    if(reed.seams){
        if(fold && !(reed.mode&runnel::decoupled)){
            Word cue=runnel::texture((ga.faces>>(2*axis))&3,(gb.faces>>(2*(axis+2)))&3);
            mask=runnel::contact(reed.stone,cue,fold-1);if(reed.mode&runnel::reversed)mask=(mask>>1)|((mask&1)<<1);
        }
        if(reed.mode&runnel::tide)boost=((a%c.side+a/c.side+tick/reed.period)&1)?6:-6;
        if(fold)count(reed.book->wet_contacts,1);
    }
    auto flow = wicks::exchange(left, right, p, q, ga, gb, axis, fibre,boost,mask);
    if(reed.seams && fold && !flow.water && mask!=3 && ga.faces && gb.faces){
        int diff=base+boost*int((2+material::weight(ga)+(p.relief&7))*(2+material::weight(gb)+(q.relief&7)));
        if(diff && !(mask&(diff<0?2:1)))count(reed.book->stopped,1);
    }
    if (!flow.water)
        return;
    auto &book = *f.ledger;
    auto *underground = w.hollows.ledger + (c.side >= 128 ? index / 128 : 0);
    count(w.account->flow, flow.water);
    count(book.contacts, 1);
    count(book.water, flow.water);
    count(book.carbon, flow.carbon);
    if (outer) {
        count(book.transverse, 1);
        count(book.transverse_food, flow.carbon);
    }
    if (f.parcels[a] & wicks::edge_bit(axis)) {
        count(book.worked, 1);
        count(book.worked_food, flow.carbon);
    }
    bool source = flow.from_right ? q.relief : p.relief, sink = flow.from_right ? p.relief : q.relief;
    if (source && !sink) {
        count(underground->released, flow.carbon);
        count(book.released, flow.carbon);
    }
    if (!source && sink) {
        count(underground->settled, flow.carbon);
        count(book.admitted, flow.carbon);
    }
    if(reed.seams && fold && (mask==1||mask==2)){
        bool marked=runnel::traced(reed.seams[a],axis),up=flow.from_right?base>0:base<0;
        if(marked){count(reed.social->water,flow.water);count(reed.social->carbon,flow.carbon);
            if(up){count(reed.social->lifted_water,flow.water);count(reed.social->lifted_carbon,flow.carbon);}}
        count(reed.book->thumped_water,flow.water);count(reed.book->thumped_carbon,flow.carbon);
        if(flow.from_right?base>0:base<0){count(reed.book->lifted_water,flow.water);count(reed.book->lifted_food,flow.carbon);}
        Word target=flow.from_right?a:b, id=w.slots[target];
        auto &animal=w.animals[id];auto &bed=w.hollows.beds[target];
        if((reed.mode&runnel::responding) && flow.carbon && animal.energy && animal.body.pores && animal.energy<65535 &&
           ((reed.minds[id].genes.hand>>(16+(w.motor[id]&7)))&1)){
            if(bed.relief){--bed.carbon;count(underground->released,1);count(underground->eaten,1);}
            else --(flow.from_right?left:right).litter;
            ++animal.energy;++animal.meals;++animal.diet[1];
            count(w.account->harvest,1);count(w.account->digested,1);count(reed.book->swallowed,1);
            if(marked){count(reed.social->meals,1);count(reed.social->swallowed,1);}
        }
    }
    auto &from = f.parcels[a], &to = f.parcels[b];
    Word prior = flow.from_right ? to : from;
    if (wicks::transport(from, to, flow.from_right)) {
        count(book.tracer_moves, 1);
        count(book.later_hops, wicks::hops(prior) != 0);
        count(book.bridged_hops, outer);
    }
}
} // namespace axion::life::flood
