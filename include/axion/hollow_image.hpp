#pragma once
#include "hollow.hpp"

namespace axion::life::hollow {
// Each bit is a cue, each plane one digit in its highest cell's priority rank.
struct Index {
    Half active = 0, planes[4]{};
    Byte slot[16]{};
};
static_assert(sizeof(Index) == 26);
AX_FN Index index(const Mind &m, View s, Word now, Word tie) {
    Index result{};
    int scores[16]{};
    for (Word i = 0; i < m.used; ++i) {
        Word e = s.at(i), cue = identity(e), bit = 1u << cue;
        if (!strength(e))
            continue;
        int score = priority(m, e, i, distance(e, m), now, tie);
        if (!(result.active & bit) || score > scores[cue]) {
            result.active |= bit;
            result.slot[cue] = i;
            scores[cue] = score;
        }
    }
    for (Word cue = 0; cue < 16; ++cue)
        if (result.active & (1u << cue)) {
            Word rank = 0;
            for (Word j = 0; j < 16; ++j)
                rank += bool(result.active & (1u << j)) && scores[j] < scores[cue];
            for (Word k = 0; k < 4; ++k)
                if (rank & (1u << k))
                    result.planes[k] |= 1u << cue;
        }
    return result;
}
AX_FN Recall indexed(const Mind &m, View s, const Index &table, Word offered, Word tie, Word blocked) {
    Word pool = table.active & offered;
    if (!pool)
        return {};
    for (int i = 3; i >= 0; --i) {
        Word upper = pool & table.planes[i];
        if (upper)
            pool = upper;
    }
    Word cue = __builtin_ctz(pool), slot = table.slot[cue], e = s.at(slot);
    return emit(m, {slot, 0, cue, 4, distance(e, m), e & 65535}, tie, blocked);
}
} // namespace axion::life::hollow
