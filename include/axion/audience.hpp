#pragma once
#include "fovea.hpp"

namespace axion::life::audience {
constexpr Word connected = 1u << 26;
constexpr Word crossed = 1u << 27, lineage = 1u << 28, field_ports = connected | crossed | lineage;
enum Arm : Word { intact, exchanged, screened, muted, reversed, disguised, hidden, private_view };
struct Ledger {
    Count meetings = 0, writes = 0, edits = 0, choices = 0, changed_hand = 0, reached = 0, trace_sensitive = 0;
};
struct Witness {
    Word visible = 0, shape = 0, face = 0, tape = 0, write_gene = 0, motor_gene = 0, exchanged = 0;
};
// Two visible outline features split silhouettes into four classes. Several
// animals can occupy the same class. Nothing here identifies a resident slot.
AX_FN Word silhouette(const flood::Anatomy &body) {
    return Word(bool(body.shape & (1u << 11))) | (Word(bool(body.shape & (1u << 7))) << 1);
}
AX_FN Word local(Word table, Word a, Word b, Word c) {
    return drift::lateral_columns(table, (a << 4) | (b << 8) | (c << 12));
}
AX_FN Word remember(regulation::Program genes, Word tape, Word face, Word onset, Word offset) {
    Word shift = 4 * face, old = (tape >> shift) & 15;
    Word next = local(genes.first, onset, old, retina::frame(offset, (genes.first >> 8) & 7));
    return (tape & ~(15u << shift)) | (next << shift);
}
AX_FN Word fire(regulation::Program genes, Word onset, Word trace, Word expected) {
    Word first = local(genes.second, onset, trace, expected);
    return local(genes.second >> 8, onset, first, retina::frame(trace, (genes.first >> 11) & 7));
}
struct Motion {
    Word first = 0, second = 0, hand = 0, opened = 0;
};
AX_FN Motion offer(regulation::Program genes, Word tape, Word face, Word a, Word b, Word expected_a, Word expected_b,
                   Word tie) {
    Word trace = (tape >> (4 * face)) & 15;
    Word left = fire(genes, a, trace, expected_a), right = fire(genes, b, trace, expected_b);
    Word x = __builtin_popcount(left), y = __builtin_popcount(right), threshold = 1 + ((genes.second >> 16) & 3);
    return {left, right, x == y ? tie & 1u : Word(y > x), Word(x >= threshold || y >= threshold)};
}
} // namespace axion::life::audience
