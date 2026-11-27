#pragma once
#include "braid_route.hpp"

namespace axion::life::braid {
enum View : Word { beginning, result, intended, shortened };
struct Episode {
    Word images = 0, known = 0, stamp = 0;
};
AX_FN Word picture(const Episode &e, Word frame) { return (e.images >> (8 * frame)) & 255; }
AX_FN Word visible(const Episode &e, Word frame) { return (e.known >> (4 * frame)) & 15; }
AX_FN void expose(Episode &e, Word frame, Word colors, Word marks) {
    e.images = (e.images & ~(255u << (8 * frame))) | ((colors & 255) << (8 * frame));
    e.known = (e.known & ~(15u << (4 * frame))) | ((marks & 15) << (4 * frame));
}
AX_FN Episode episode(const Mind &m, Word start, Word after, Prediction aim, Plan pull, Word witness, Word stamp) {
    Episode e{};
    e.stamp = stamp ? stamp : 1;
    expose(e, beginning, start, 15);
    expose(e, result, after, 15);
    if (!witness) {
        expose(e, intended, aim.colors, aim.known);
        Word path = identity;
        for (Word j = 0; j + 1 < pull.size; ++j)
            path = compose(matrix(m, (pull.stream >> (3 * j)) & 7), path);
        auto prefix = predict(path, start);
        expose(e, shortened, prefix.colors, prefix.known);
    }
    return e;
}
AX_FN bool valid_episode(const Episode &e) { return !(e.known >> 16) && (e.stamp || (!e.images && !e.known)); }
} // namespace axion::life::braid
