#pragma once
#include "axion/linkage_design.hpp"
#include "linkage_cells.cuh"
#include "linkage_physics.cuh"
namespace axion::life::linkage {
AX_FN Design solve_fixed(const Map &map, Rig r, Word start, Pattern goal, Genes g, Word attention = 0) {
    auto p = attend(map, r, start, goal, g, attention);
    return {p.course, no_edit, p.found, p.course.len, start};
}
AX_FN bool consider(Design &best, Plan route, Word edit) {
    if (!route.found)
        return false;
    Word price = route.course.len + 2;
    if (best.found && best.effort <= price)
        return false;
    best = {route.course, edit, 1, price, 0};
    return true;
}
AX_FN bool model_splice(const Map &m, Rig &r, Word slot) {
    Word a = edge_a(slot), b = edge_b(slot), old = joint(r.joints, a, b), g = bits(r.glyphs, a);
    Word relation = apply(m.splice[g], old, 1);
    if (relation > 2 || relation == old)
        return false;
    join(r, a, b, relation);
    return true;
}
AX_FN Design design(const Map &map, Rig r, Word start, Pattern goal, Word sockets, Genes g) {
    auto best = solve_fixed(map, r, start, goal, g);
    if (best.found && best.effort <= 2)
        return best;
    for (Word avail = sockets; avail; avail &= avail - 1) {
        Word slot = __builtin_ctz(avail);
        auto altered = r;
        if (!model_splice(map, altered, slot))
            continue;
        consider(best, seek(map, altered, start, goal, g), slot);
    }
    return best;
}
AX_FN bool insert(Rig &r, Word sockets, Word slot, Word &loop, const Material *law, Word budget) {
    if (slot >= no_edit || !(sockets & (1u << slot)) || !loop || budget < 2)
        return false;
    Word a = edge_a(slot), b = edge_b(slot), g = bits(r.glyphs, a);
    join(r, a, b, thread(law[g], joint(r.joints, a, b)));
    --loop;
    return true;
}
} // namespace axion::life::linkage
