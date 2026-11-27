#pragma once
#include "narrative.hpp"

namespace axion::life::utterance {
constexpr Word fallback = 1u << 31;
struct Chip {
    Word duration = 0, lengths = 0, motor[shapes][phases]{}, seams[styles]{}, rejected = 0, pairs = 0;
};
AX_FN Word compiled_drive(Word cell, Word light) {
    Word channel_id = (cell >> 24) & 7;
    return (cell >> (6 * channel(light, channel_id))) & 63;
}
// A four-way read can replace an acquired projection only when it preserves
// its exact output, including silence, on every possible sensory volley.
AX_FN Word compile(const Socket &s, const Socket *duration = nullptr, Word length = 0) {
    if (!s.fixed && !s.row[0] && !s.row[1] && !s.row[2] && !s.row[3] && !s.row[4] && !s.row[5] && !s.row[6] &&
        !s.row[7])
        return 0;
    for (Word k = 0; k < fields; ++k) {
        Word code = k << 24, seen = 0;
        bool good = true;
        for (Word e = 0; e < 256; ++e) {
            Word light = texture(e);
            if (duration && drive(*duration, light) != length)
                continue;
            Word at = channel(light, k), value = drive(s, light);
            if (seen & (1u << at)) {
                if (compiled_drive(code, light) != value) {
                    good = false;
                    break;
                }
            } else {
                code |= value << (6 * at);
                seen |= 1u << at;
            }
        }
        if (good)
            return code;
    }
    return fallback;
}
AX_FN Word activate_motor(Word selector, const Socket *residual, Word light) {
    return selector & fallback ? drive(residual[selector & ~fallback], light) : compiled_drive(selector, light);
}
AX_FN Packet compact(const Chip &chip, const Socket *residual, Word e) {
    Word light = texture(e), len = activate_motor(chip.duration, residual, light);
    if (len)
        for (Word j = 0; j < shapes; ++j)
            if (((chip.lengths >> (4 * j)) & 15) == len) {
                Count bits = Count(len) << 56;
                for (Word i = 0; i < len; ++i) {
                    Word gate = chip.motor[j][i], value = activate_motor(gate, residual, light);
                    if (!value)
                        return {};
                    bits |= Count(value) << (6 * i);
                }
                return {bits};
            }
    return {};
}
} // namespace axion::life::utterance
