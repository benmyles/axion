#pragma once
#include "axion/practice_words.hpp"
#include "practice_policy.cuh"
namespace axion::life::practice {
AX_FN Card encode(Script p) {
    Card c{};
    c.bytes[c.size++] = p.found ? static_cast<unsigned char>(p.ask | ((p.look & 3) << 4)) : 15;
    if (!p.found)
        return c;
    if (p.ask < 8)
        c.bytes[c.size++] = p.outcomes;
    for (Word b = 0; b < 4; ++b)
        if (p.outcomes & (1u << b)) {
            auto &r = p.reply[b];
            c.bytes[c.size++] = static_cast<unsigned char>((r.edit & 7) | (r.turns.len << 3));
            Count bits = Count(r.turns.low) | (Count(r.turns.high) << 24);
            for (Word j = 0; j < 3 * r.turns.len; j += 8)
                c.bytes[c.size++] = bits >> j;
        }
    return c;
}
AX_FN linkage::Design decode(const unsigned char *code, Word report) {
    linkage::Design p{};
    Word first = code[0] & 15;
    if (first == 15)
        return p;
    Word offset = 1, arms = 1;
    if (first < 8)
        arms = code[offset++];
    else
        report = 0;
    if (report > 3 || !(arms & (1u << report)))
        return p;
    for (Word b = 0; b < 4; ++b)
        if (arms & (1u << b)) {
            Word header = code[offset++], len = header >> 3, bytes = (3 * len + 7) / 8;
            if (b == report) {
                Count v = 0;
                for (Word j = 0; j < bytes; ++j)
                    v |= Count(code[offset + j]) << (8 * j);
                p.turns = {Word(v) & 0xffffffu, Word(v >> 24), len};
                p.edit = header & 7;
                p.found = 1;
                p.effort = len + 2 * Word(p.edit < 6);
                return p;
            }
            offset += bytes;
        }
    return p;
}
} // namespace axion::life::practice
