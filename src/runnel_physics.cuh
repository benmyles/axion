#pragma once
#include "axion/flume.hpp"
namespace axion::life::runnel {
AX_FN Word discharge(int difference, Word scale, Word conductance, Word mask, Word stock, Word space) {
    if (!difference || !(mask & (difference < 0 ? 2u : 1u)))
        return 0;
    Word flow = Word(difference < 0 ? -difference : difference) / scale;
    if (flow > conductance)
        flow = conductance;
    if (flow > stock)
        flow = stock;
    return flow < space ? flow : space;
}
AX_FN Word dissolved(Word stream, Word stock, Word room) {
    Word n = stream < stock ? stream : stock;
    return n < room ? n : room;
}
AX_FN Word flex(Word stone, Word cue, Word public_pose) {
    Word mask = contact(stone, cue, public_pose);
    // Two opposed unit loads on a newly bent tongue. The bystander sees
    // its two deflections; contact() itself is inaccessible to the learner.
    return discharge(2, 2, 1, mask, 1, 1) | (discharge(-2, 2, 1, mask, 1, 1) << 1);
}
AX_FN Word head(const Chamber &c, Word i, Word edges, Word t, bool moving) {
    if (i == 0)
        return 5;
    if (i == edges)
        return 17;
    return c.water + ((moving && (((t >> 2) ^ i) & 1)) ? 24u : 0u);
}
AX_FN Word sluice(Chamber &a, Chamber &b, int difference, Word mask, Word &volume) {
    auto &src = difference < 0 ? b : a, &dst = difference < 0 ? a : b;
    volume = discharge(difference, 2, 3, mask, src.water, 65535u - dst.water);
    Word carbon = dissolved(volume, src.carbon, 65535u - dst.carbon);
    src.water -= volume;
    dst.water += volume;
    src.carbon -= carbon;
    dst.carbon += carbon;
    return carbon;
}
AX_FN void fill(Chamber *cells, Word edges) {
    for (Word i = 1; i < edges; ++i)
        cells[i] = {5, 0};
    cells[0] = {4096, 4096};
    cells[edges] = {0, 0};
}
AX_FN void advance(Chamber *cells, const Byte *valves, Word edges, Word first, Word rounds, bool moving, Gallery &q) {
    for (Word k = 0; k < rounds; ++k) {
        Word t = first + k;
        for (Word half = 0; half < 2; ++half)
            for (Word i = half ^ (t & 1); i < edges; i += 2) {
                int delta = int(head(cells[i], i, edges, t, moving)) - int(head(cells[i + 1], i + 1, edges, t, moving));
                Word volume;
                auto carbon = sluice(cells[i], cells[i + 1], delta, valves[i], volume);
                if (i == 0 && delta > 0)
                    q.from += carbon;
                if (i + 1 == edges) {
                    if (delta > 0)
                        q.into += carbon;
                    else
                        q.back += carbon;
                }
            }
    }
}
AX_FN Gallery pump(const Byte *valves, Word edges, Word ticks, bool moving) {
    Chamber cells[max_span + 1];
    fill(cells, edges);
    Gallery q{};
    for (Word i = 0; i < edges; ++i) {
        q.positive += valves[i] == 1;
        q.negative += valves[i] == 2;
        q.free += valves[i] == 3;
        q.closed += valves[i] == 0;
    }
    advance(cells, valves, edges, 0, ticks, moving, q);
    for (Word i = 0; i <= edges; ++i) {
        q.water_total += cells[i].water;
        q.carbon_total += cells[i].carbon;
        if (cells[i].carbon)
            q.highest = i;
    }
    q.water = cells[edges].water;
    q.food = cells[edges].carbon;
    return q;
}
} // namespace axion::life::runnel
