#pragma once
#include "capillary.hpp"

namespace axion::life::capillary {
struct Sheet {
    Count wick = 0, wet = 0, sill = 0, paint = 0;
};
AX_FN Count &surface(Sheet &s, Word layer) {
    return layer == 0 ? s.wick : layer == 1 ? s.wet : layer == 2 ? s.sill : s.paint;
}
AX_FN Word texture(const Sheet &s, Word at) {
    Count b = bit(at);
    return Word(bool(s.wick & b)) | (Word(bool(s.wet & b)) << 1) | (Word(bool(s.sill & b)) << 2) |
           (Word(bool(s.paint & b)) << 3);
}
struct Bounds {
    Count on = 0, maybe = 0;
};
constexpr Count first_column = 0x0101010101010101ull;
AX_FN Count from_west(Count state) {
    return (state << 1) & ~first_column;
}
AX_FN Count from_north(Count state) {
    return state << 8;
}
AX_FN Word address(Count past, const Sheet &s, Word j) {
    return (texture(s, j) << 2) | Word(bool(from_west(past) & bit(j))) | (Word(bool(from_north(past) & bit(j))) << 1);
}
AX_FN Count physical(const Sheet &s, Count state, Word mineral) {
    Count a = from_west(state), b = from_north(state);
    Count single_a = (!((mineral >> 2) & 1) ? 0 : full), single_b = (!((mineral >> 3) & 1) ? 0 : full);
    Count weak = a | b, twice = (a & b) | (a & single_a) | (b & single_b);
    Count three = (a & b) & (single_a | single_b);
    Count adhesive = (mineral & 16) ? s.paint : s.wick;
    Count easy = weak & adhesive;
    Count high = (mineral & 2) ? (three | (twice & adhesive) | ((mineral & 1) ? easy : 0)) : (twice | easy);
    return s.wet & ((weak & ~s.sill) | (high & s.sill));
}
struct Projection {
    Count on[4]{}, off[4]{};
};
AX_FN void install(Projection &image, Model m, Word senses, Word at, Word texture4) {
    Count place = bit(at);
    for (Word inlet = 0; inlet < 4; ++inlet) {
        Count condition = bit((inlet | (texture4 << 2)) & senses);
        image.on[inlet] = (image.on[inlet] & ~place) | ((m.on & condition) ? place : 0);
        image.off[inlet] = (image.off[inlet] & ~place) | ((m.off & condition) ? place : 0);
    }
}
AX_FN Projection project(Model m, Word senses, const Sheet &s) {
    Projection map{};
    // Each material combination is shared by all sites carrying it.
    for (Word material = 0; material < 16; ++material) {
        Count at = ((material & 1) ? s.wick : ~s.wick) & ((material & 2) ? s.wet : ~s.wet) &
                   ((material & 4) ? s.sill : ~s.sill) & ((material & 8) ? s.paint : ~s.paint);
        if (!at)
            continue;
        for (Word p = 0; p < 4; ++p) {
            Count signal = bit((p | (material << 2)) & senses);
            if (m.on & signal)
                map.on[p] |= at;
            if (m.off & signal)
                map.off[p] |= at;
        }
    }
    return map;
}
AX_FN Bounds imagine(const Projection &m, Bounds s) {
    Count al = from_west(s.on), au = from_west(s.maybe), bl = from_north(s.on), bu = from_north(s.maybe);
    Count possible[]{~al & ~bl, au & ~bl, ~al & bu, au & bu};
    Count yes = full, no = full;
    for (Word j = 0; j < 4; ++j) {
        yes &= m.on[j] | ~possible[j];
        no &= m.off[j] | ~possible[j];
    }
    return {yes, ~no};
}
// On a completely identified material, four 64-bit coefficient planes
// evaluate all 64 heterogeneous junctions with the same word instructions.
struct Die {
    Count bias = 0, a = 0, b = 0, joint = 0;
};
AX_FN Die stamp(const Projection &image) {
    return {image.on[0], image.on[1] ^ image.on[0], image.on[2] ^ image.on[0],
            image.on[3] ^ image.on[2] ^ image.on[1] ^ image.on[0]};
}
AX_FN Count propagate(Die im, Count state) {
    Count a = from_west(state), b = from_north(state);
    return im.bias ^ (a & im.a) ^ (b & im.b) ^ (a & b & im.joint);
}
struct Tool {
    Byte site = 0, material = 0;
};
constexpr Word grips = 6;
constexpr Word standard_stock = 0x66666u;
struct Work {
    Sheet bed{};
    Count feed = 0, catcher = 0, waste = 0;
    Tool handle[grips]{};
    Word n = 0, time = 0, stock = standard_stock;
};
AX_FN Sheet assemble(const Work &w, Word muscles) {
    auto bed = w.bed;
    for (Word j = 0; j < w.n; ++j)
        if ((muscles & (1u << j)) && w.handle[j].material < 4)
            surface(bed, w.handle[j].material) ^= bit(w.handle[j].site);
    return bed;
}
AX_FN Projection altered(const Work &w, Projection image, Model learned, Word senses, Word muscles) {
    if (!muscles)
        return image;
    auto bed = assemble(w, muscles);
    for (Word j = 0; j < w.n; ++j)
        if ((muscles & (1u << j)) && w.handle[j].material < 4)
            install(image, learned, senses, w.handle[j].site, texture(bed, w.handle[j].site));
    return image;
}
AX_FN Count source(const Work &w, Word moves) {
    Count feed = w.feed;
    for (Word j = 0; j < w.n; ++j)
        if ((moves & (1u << j)) && w.handle[j].material >= 4)
            feed ^= bit(w.handle[j].site);
    return feed;
}
AX_FN Bounds forecast(const Work &w, const Projection &im, Word moves = 0) {
    Count start = source(w, moves);
    Bounds s{start, start};
    for (Word i = 0; i < w.time; ++i)
        s = imagine(im, s);
    return s;
}
AX_FN Bounds project_plan(const Work &w, const Projection &im, bool identified, Word moves) {
    if (!identified)
        return forecast(w, im, moves);
    Count pulse = source(w, moves);
    auto die = stamp(im);
    for (Word j = 0; j < w.time; ++j)
        pulse = propagate(die, pulse);
    return {pulse, pulse};
}
AX_FN Count release(const Work &w, Word muscles, Word mineral) {
    Sheet bed = assemble(w, muscles);
    Count pulse = source(w, muscles);
    for (Word i = 0; i < w.time; ++i)
        pulse = physical(bed, pulse, mineral);
    return pulse;
}
AX_FN Word target(const Work &w, Bounds result) {
    if ((w.catcher & ~result.maybe) || (w.waste & result.on))
        return 0;
    if (!(w.catcher & ~result.on) && !(w.waste & result.maybe))
        return 1;
    return absent;
}
// The rack has six levers. Each command moves one physical portion between a seat and a loose stock.
struct Protocol {
    Word order = 0, n = 0;
    AX_FN Word at(Word i) const {
        return (order >> (3 * i)) & 7;
    }
    AX_FN void append(Word j) {
        order |= j << (3 * n++);
    }
};
AX_FN Word substance(Word material) {
    return material < 4 ? material : 4;
}
AX_FN Word portions(Word stock, Word kind) {
    return (stock >> (4 * kind)) & 15;
}
AX_FN Count layer(const Sheet &s, Word j) {
    return j == 0 ? s.wick : j == 1 ? s.wet : j == 2 ? s.sill : s.paint;
}
AX_FN bool loaded(const Work &w, Word j, Word mask = 0) {
    auto h = w.handle[j];
    Word kind = substance(h.material);
    bool filled = bool((kind == 4 ? w.feed : layer(w.bed, kind)) & bit(h.site));
    for (Word k = 0; mask; mask &= mask - 1) {
        k = __builtin_ctz(mask);
        auto g = w.handle[k];
        filled ^= g.site == h.site && substance(g.material) == kind;
    }
    return filled;
}
struct Receipt {
    Word position = 0, supply = 0, moved = 0, denied = 0;
};
AX_FN bool stroke(const Work &w, Receipt &r, Word peg) {
    if (peg >= w.n) {
        ++r.denied;
        return false;
    }
    Word kind = substance(w.handle[peg].material), unit = 1u << (4 * kind);
    if (loaded(w, peg, r.position))
        r.supply += unit;
    else if (portions(r.supply, kind))
        r.supply -= unit;
    else {
        ++r.denied;
        return false;
    }
    r.position ^= 1u << peg;
    ++r.moved;
    return true;
}
AX_FN Receipt perform(const Work &w, Protocol seq, bool backward = false, bool misplaced = false) {
    Receipt r{0, w.stock};
    for (Word k = 0; k < seq.n; ++k) {
        Word j = seq.at(backward ? seq.n - 1 - k : k);
        stroke(w, r, misplaced ? (j + 1) % w.n : j);
    }
    return r;
}
AX_FN Protocol order(const Work &w, Word moves) {
    Protocol seq{};
    Word placed = 0;
    for (Word pass = 0; pass < 2; ++pass)
        for (Word bits = moves; bits; bits &= bits - 1) {
            Word j = __builtin_ctz(bits), mask = 1u << j;
            if (!(placed & mask) && (pass || loaded(w, j, placed))) {
                seq.append(j);
                placed |= mask;
            }
        }
    return seq;
}
AX_FN bool stocked(const Work &w, Word m) {
    if (w.stock == standard_stock)
        return true;
    // Each lever handles one substance. A removal never needs an empty hand in the shared rack.
    auto changed = assemble(w, m);
    for (Word k = 0; k < 4; ++k)
        if (count(layer(changed, k)) > count(layer(w.bed, k)) + portions(w.stock, k))
            return false;
    return count(source(w, m)) <= count(w.feed) + portions(w.stock, 4);
}
struct Plan {
    Word motions = 0, ready = 0, alternatives = 0, tried = 0;
};
AX_FN Plan design(const Work &w, Model model, Word senses, Word freedom, Word limit) {
    auto base = project(model, senses, w.bed);
    Plan p{};
    Word shortest = limit + 1;
    for (Word move = 0; move < (1u << w.n); ++move) {
        Word work = __builtin_popcount(move);
        if ((move & ~freedom) || work > limit || work > shortest || !stocked(w, move))
            continue;
        ++p.tried;
        if (target(w, project_plan(w, altered(w, base, model, senses, move), (model.on | model.off) == full, move)) !=
            1)
            continue;
        if (work < shortest) {
            shortest = work;
            p = {move, 1, 1, p.tried};
        } else
            ++p.alternatives;
    }
    return p;
}
} // namespace axion::life::capillary
