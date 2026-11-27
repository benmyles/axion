#pragma once
#include "runnel_analogy.cuh"
namespace axion::life::flood {
AX_FN void feel_reed(World w, Configuration c, Word id, Word other, Word cue, Word gesture, Word desired,
                     runnel::Choice pick) {
    auto f = w.wicks.reeds;
    auto &body = w.animals[id];
    auto &mind = f.minds[id];
    auto g = mind.genes;
    --body.energy;
    ++body.worked;
    count(w.account->heat, 1);
    count(f.book->work, 1);
    count(f.book->probes, 1);
    Word effect = runnel::flex(f.stone, cue, gesture), mode = c.plastic ? f.mode : f.mode & ~runnel::sensing;
    count(f.book->predicted, pick.known);
    count(f.book->accurate, pick.known && pick.expected == effect);
    count(f.book->selected, pick.known && effect == (1u << desired));
    count(f.book->personal_writes, runnel::observe(mind, cue, gesture, g.hand & 7, effect, false, mode));
    auto &peer = w.animals[other];
    auto &observer = f.minds[other];
    bool can_watch = (f.mode & runnel::compete_reason) ||
                     runnel::entitled(f.mode, w.hollows.minds[other].efferent & hollow::lineage_tag);
    if (peer.energy && peer.body.eyes && (f.mode & runnel::watching) && can_watch) {
        count(f.book->peered, 1);
        count(f.book->social_writes,
              runnel::observe(observer, cue, gesture, observer.genes.hand & 7, effect, true, mode));
    }
}
AX_FN void fold_reed(World w, Configuration c, Word a, Word b, Word axis, Word key) {
    auto f = w.wicks.reeds;
    if (!f.minds || !(f.mode & runnel::forming) || !c.weaving || (key & 7))
        return;
    auto ga = w.matter.bed[a], gb = w.matter.bed[b];
    if (!ga.faces || !gb.faces)
        return;
    Word ends[]{a, b}, cue = runnel::texture((ga.faces >> (2 * axis)) & 3, (gb.faces >> (2 * (axis + 2))) & 3);
    auto &strand = axis ? w.land[a].fibre_south : w.land[a].fibre_east;
    for (Word q = 0; q < 2; ++q) {
        Word side = q ^ ((key >> 9) & 1), id = w.slots[ends[side]], other = w.slots[ends[side ^ 1]];
        auto &body = w.animals[id];
        auto &mind = f.minds[id];
        auto g = mind.genes;
        if (body.energy < 5 + ((g.hand >> 24) & 7) || !body.body.eyes || !body.body.limbs ||
            !((g.hand >> (16 + (w.motor[id] & 7))) & 1))
            continue;
        if (f.mode & runnel::patrolling) {
            Word home = ends[side], facing = ends[side ^ 1];
            bool home_food = w.hollows.beds[home].relief ? w.hollows.beds[home].carbon != 0 : w.land[home].litter != 0;
            bool next_food =
                w.hollows.beds[facing].relief ? w.hollows.beds[facing].carbon != 0 : w.land[facing].litter != 0;
            Word move =
                runnel::gait(g, home_food, next_food, runnel::fold_at(f.seams[a], axis) != 0, mix(key + id * 9959));
            if (move == 3) {
                auto current = runnel::fold_at(f.seams[a], axis);
                if (!current)
                    continue;
                Word motor = runnel::native(current - 1, g.hand & 7), bit = 1u << motor;
                auto q = runnel::profile(mind, cue);
                runnel::Choice pick{motor, Word(!!(q.touched & bit)), 0,
                                    Word(!!(q.forward & bit)) + 2 * Word(!!(q.backward & bit))};
                count(f.book->opportunities, 1);
                feel_reed(w, c, id, other, cue, current - 1, (side ^ 1) ^ ((g.hand >> 28) & 1), pick);
                return;
            }
            if (move == 1 || move == 2) {
                Word facing_dir = (axis ? 2u : 1u) ^ (side ? 2u : 0u);
                body.heading = Byte(move == 1 ? facing_dir : facing_dir ^ 2);
                continue;
            }
        }
        count(f.book->opportunities, 1);
        if (!strand && !body.fibre) {
            auto &ground = w.land[ends[side]];
            if (!ground.litter || !(g.hand & (1u << 27)) ||
                body.load + body.fibre >= 2 + 2 * __builtin_popcount(body.body.limbs))
                continue;
            --ground.litter;
            ++body.fibre;
            --body.energy;
            ++body.worked;
            count(w.account->fibre_lifted, 1);
            count(w.account->heat, 1);
            count(f.book->work, 1);
            return;
        }
        Word desired = (side ^ 1) ^ ((g.hand >> 28) & 1), roll = mix(key + id * 2477);
        bool reason = (f.mode & runnel::relating) &&
                      (!(f.mode & runnel::compete_reason) ||
                       runnel::entitled(f.mode, w.hollows.minds[id].efferent & hollow::lineage_tag));
        auto projection = runnel::infer(mind, cue, desired, roll, true, reason);
        auto pick = projection.action;
        count(f.book->tasted, projection.derived);
        Word gesture = runnel::pose(pick.motor, g.hand & 7), current = runnel::fold_at(f.seams[a], axis);
        if (current == gesture + 1 && (g.hand & (1u << 29)) && (roll & 3))
            return;
        if (!strand) {
            --body.fibre;
            ++strand;
            tag_strand(w, c.side, a, b, 0, 1);
            count(w.account->fibre_laid, 1);
        }
        Word mask = runnel::contact(f.stone, cue, gesture);
        bool right = mask == (1u << desired), traced = current == gesture + 1 && runnel::traced(f.seams[a], axis);
        if (pick.known && (mind.cells.witnessed[0] || mind.cells.witnessed[1])) {
            count(f.social->observed_makers, 1);
            auto hidden = mind;
            for (Word h = 0; h < 2; ++h) {
                Word keep = ~hidden.cells.witnessed[h];
                hidden.cells.touched[h] &= keep;
                hidden.cells.forward[h] &= keep;
                hidden.cells.backward[h] &= keep;
                hidden.cells.witnessed[h] = 0;
            }
            auto alternate = runnel::infer(hidden, cue, desired, roll, true, reason).action.motor;
            if (alternate != pick.motor) {
                bool would = runnel::contact(f.stone, cue, runnel::pose(alternate, g.hand & 7)) == (1u << desired);
                count(f.social->altered, 1);
                count(f.social->better, right && !would);
                count(f.social->worse, !right && would);
                if (right && !would && current != gesture + 1) {
                    traced = true;
                    count(f.social->marked, 1);
                }
            }
        }
        if (projection.derived) {
            auto alternate = runnel::choose(mind, cue, desired, roll, true).motor;
            bool would = runnel::contact(f.stone, cue, runnel::pose(alternate, g.hand & 7)) == (1u << desired);
            count(f.social->inferred_changes, alternate != pick.motor);
            count(f.social->inferred_better, right && !would);
            count(f.social->inferred_worse, !right && would);
        }
        runnel::set_fold(f.seams[a], axis, gesture + 1 + (traced ? 8 : 0));
        --body.energy;
        ++body.worked;
        count(w.account->heat, 1);
        count(f.book->work, 1);
        count(f.book->folds, 1);
        count(f.book->created, !current);
        count(f.book->retuned, current && current != gesture + 1);
        count(f.book->recalled, pick.known);
        count(f.book->social_use, pick.from_peer);
        if (!(g.eye & (1u << 5)) && (roll & 7) > ((g.eye >> 16) & 7))
            return;
        feel_reed(w, c, id, other, cue, gesture, desired, pick);
        return;
    }
}
} // namespace axion::life::flood
