#pragma once
#include "axion/wicks.hpp"
#include "runnel_physics.cuh"
namespace axion::life::wicks {
// Relative saturation at the two mineral interfaces sets the direction.
// A fibrous bridge remains permeable when the two mechanical faces differ,
// at a smaller conductance. Every transferred unit leaves a real donor.
AX_FN Passage exchange(flood::Silt &a, flood::Silt &b, hollow::Bed &left, hollow::Bed &right, material::Grain ga,
                       material::Grain gb, Word axis, Word fibre, int pressure=0, Word apertures=3) {
    Passage p{};
    if (!fibre || !ga.faces || !gb.faces)
        return p;
    Word wall = axis ? a.south : a.east;
    Word face_a = (ga.faces >> (2 * axis)) & 3, face_b = (gb.faces >> (2 * (axis + 2))) & 3;
    Word contact = 1 + (face_a == face_b) + Word(fibre >= 3) + Word(fibre >= 9);
    if (contact + fibre + material::weight(ga) + material::weight(gb) <= wall)
        return p;
    Word ca = 2 + material::weight(ga) + (left.relief & 7), cb = 2 + material::weight(gb) + (right.relief & 7);
    int imbalance = int(Word(a.water) * cb) - int(Word(b.water) * ca)+pressure*int(ca*cb);
    if (!imbalance)
        return p;
    bool reverse = imbalance < 0;
    auto &donor = reverse ? b : a, &receiver = reverse ? a : b;
    auto &well = reverse ? right : left, &sink = reverse ? left : right;
    Word water=runnel::discharge(imbalance,ca+cb,contact,apertures,donor.water,65535u-receiver.water);
    if(!water)return p;
    auto &food = well.relief ? well.carbon : donor.litter;
    auto &store = sink.relief ? sink.carbon : receiver.litter;
    Word capacity = sink.relief ? 255 : 65535;
    Word solute = runnel::dissolved(water,food,capacity-store);
    donor.water -= water;
    receiver.water += water;
    food -= solute;
    store += solute;
    return {water, solute, 1, reverse};
}
AX_FN bool transport(Word &a, Word &b, bool reverse) {
    Word &source = reverse ? b : a, &dest = reverse ? a : b;
    if (!(source & present) || (dest & present))
        return false;
    Word h = hops(source);
    dest = (dest & ~trail) | label(origin(source), h < 127 ? h + 1 : h);
    source &= ~trail;
    return true;
}
} // namespace axion::life::wicks
