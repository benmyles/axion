#include "practice_policy.cuh"
namespace axion::life::practice {
struct Patch {
    Request order{};
    linkage::Material laws[2][4]{};
    Word climates[2]{};
};
AX_FN void climate(Word seed, Word *names, linkage::Material (&laws)[2][4]) {
    names[0] = mix(seed + 7807);
    names[1] = mix(seed + 19121);
    for (Word m = 0; m < 2; ++m)
        for (Word j = 0; j < 4; ++j)
            laws[m][j] = linkage::matter(names[m], j);
}
AX_FN Word position(Rig rack, Word visible, const linkage::Material *law) {
    Word angle = 0;
    for (Word i = 0; i < 4; ++i)
        if (rack.parts & (1u << i)) {
            Word g = bits(rack.glyphs, i);
            for (Word a = 0; a < 4; ++a)
                if (linkage::pigment(law[g], a) == bits(visible, i))
                    angle = replace(angle, i, a);
        }
    return angle;
}
AX_FN Request apparatus(Word key, Word width, const linkage::Material *law) {
    Request r{};
    auto s = linkage::yard(key, width, law);
    r.rig = s.frame;
    r.sockets = s.sockets;
    r.view = linkage::expose(s.frame, s.pose, law);
    r.goal = s.target;
    r.work = 3 + ((mix(key + 4091) >> 16) % 5);
    // Partial work orders leave real manipulable distractors in the apparatus.
    if ((key & 7) == 0 && width > 1)
        r.goal.care &= ~(3u << (2 * ((key >> 9) % width)));
    return r;
}
AX_FN void contact(Mind &m, Slots s, Rig r, Word &pose, Word motor, const linkage::Material *law, bool watched,
                   Word mode) {
    Word v = linkage::expose(r, pose, law);
    pose = linkage::press(r, pose, motor, law);
    observe(m, s, r, v, motor, linkage::expose(r, pose, law), watched, mode);
}
AX_FN Word handle(Mind &m, Slots cells, const linkage::Material *law, Word seed, Word rounds, Word condition,
                  Word mode, Word kinds = 15) {
    Word expense = 0;
    bool witness = condition != own_contacts;
    for (Word t = 0; t < rounds; ++t) {
        for (Word i = 0; i < 32; ++i) {
            Word k = (i * (1 + 2 * (seed & 7)) + (seed >> 9) + t) & 31, g = k >> 3;
            if (!(kinds & (1u << g)))
                continue;
            Rig r{1, g, 0, 3};
            Word pose = k & 3;
            Word act = condition == unguided
                           ? inquiry(m, cells, r, linkage::expose(r, pose, law), mix(seed + i + 41 * t), mode)
                           : (k >> 2) & 1;
            contact(m, cells, r, pose, act, law, witness, mode);
            ++expense;
        }
        for (Word i = 0; i < 64; ++i) {
            Word k = (i * (1 + 2 * ((seed >> 3) & 15)) + (seed >> 17) + t) & 63, g = k >> 4;
            if (!(kinds & (1u << g)))
                continue;
            Word other = linkage::bump(kinds, mix(seed + 1171 * k)), pose = (k & 3) | (mix(seed + 73 * i) & 12);
            Rig r{3, g | (other << 2), 0, 15};
            linkage::join(r, 0, 1, 1 + ((k >> 3) & 1));
            Word act = condition == unguided
                           ? inquiry(m, cells, r, linkage::expose(r, pose, law), mix(seed + 31 * i), mode)
                           : (k >> 2) & 1;
            contact(m, cells, r, pose, act, law, witness, mode);
            ++expense;
        }
        for (Word i = 0; i < 12; ++i) {
            Word g = i % 4, old = i / 4;
            if (!(kinds & (1u << g)))
                continue;
            Rig r{3, g | (g << 2), 0, 3};
            linkage::join(r, 0, 1, old);
            Word fibre = 1;
            linkage::insert(r, 1, 0, fibre, law, 2);
            fitting(m, cells, g, old, joint(r.joints, 0, 1), witness, mode);
            expense += 2;
            linkage::join(r, 0, 1, old);
            ++fibre;
            ++expense;
        }
    }
    return expense;
}
AX_FN Word execute(const Request &r, linkage::Design p, Word start, const linkage::Material *law, Word &paid,
                   Word &threaded) {
    Rig rig = r.rig;
    Word fibre = 1;
    Word pose = position(rig, start, law);
    if (!p.found || p.effort + paid > r.work)
        return start;
    if (p.edit < linkage::no_edit) {
        if (!linkage::insert(rig, r.sockets, p.edit, fibre, law, r.work - paid))
            return start;
        paid += 2;
        threaded = 1;
    }
    for (Word i = 0; i < p.turns.len; ++i) {
        Word act = linkage::step(p.turns, i);
        if (!(rig.handles & (1u << act)))
            return linkage::expose(rig, pose, law);
        pose = linkage::press(rig, pose, act, law);
        ++paid;
    }
    return linkage::expose(rig, pose, law);
}
AX_FN Score challenge(const Mind &m, const Charts &models, Request r, const linkage::Material *law, Word arm,
                      Word seed, bool grading, Specimen *proof) {
    auto tree = reason(m, models, r, r.rig.handles, seed,
                       arm == no_test   ? all & ~Word(probing)
                       : arm == placebo ? all | random_probes
                                        : all);
    Score q{};
    q.trials = 1;
    q.issued = tree.found;
    q.active_contexts = models.count;
    q.abstained = !tree.found;
    Word here = r.view, spoken = 4, replay = 4;
    if (tree.found) {
        Word branch = 0;
        if (tree.ask < 8) {
            Word pose = position(r.rig, here, law);
            pose = linkage::press(r.rig, pose, tree.ask, law);
            here = linkage::expose(r.rig, pose, law);
            ++q.paid;
            q.questions = 1;
            q.indirect = (tree.ask >> 1) != tree.look;
            spoken = bits(here, tree.look);
            branch = spoken;
            if (arm == silence)
                branch = 4;
            if (arm == shifted)
                branch = (branch + 1) & 3;
            replay = branch;
            q.answers = (branch < 4 && (tree.outcomes & (1u << branch))) != 0;
            q.unneeded = linkage::matches(here, r.goal);
            q.branches = __builtin_popcount(tree.outcomes) > 1;
        } else
            q.unconditional = 1;
        if (branch < 4 && (tree.outcomes & (1u << branch)))
            here = execute(r, tree.reply[branch], here, law, q.paid, q.splices);
    }
    q.delivered = linkage::matches(here, r.goal);
    q.mistakes = q.issued && !q.delivered;
    q.harvest = q.delivered * 40;
    q.good_branches = q.delivered * q.questions;
    q.instances = q.delivered && here != r.view;
    if (grading) {
        auto truth = linkage::true_map(law);
        auto g = motor_genes(m.genes);
        g.impulse |= 15u << 8;
        if (auto optimum = linkage::design(truth, r.rig, r.view, r.goal, r.sockets, g);
            optimum.found && optimum.effort <= r.work)
            q.possible = 1;
        // A diagnostic motion can precede attachment. Include this legal work in the reference.
        for (Word mask = r.rig.handles; !q.possible && mask; mask &= mask - 1) {
            Word motor = __builtin_ctz(mask);
            Word next = linkage::predict(truth, r.rig, r.view, motor);
            auto p = linkage::design(truth, r.rig, next, r.goal, r.sockets, g);
            q.possible = p.found && p.effort + 1 <= r.work;
        }
    }
    if (proof) {
        proof->request = r;
        proof->script = tree;
        proof->final = here;
        proof->report = spoken;
        proof->alternate = replay;
    }
    return q;
}
AX_FN Score school(Genes genes, Word key, Word width, Word cond, Word cap, bool grade, Specimen *spec) {
    Word worlds[2];
    linkage::Material law[2][4];
    climate(key, worlds, law);
    Mind m{genes};
    Context tissues[4]{};
    Slots s{tissues, 1, cond == merged ? 1u : cap};
    Word work = 0, switches = 0;
    if (cond != erased) {
        for (Word phase = 0; phase < (cond == one_exposure ? 1u : 2u); ++phase) {
            if (cond == discarded && phase) {
                m = {genes};
                for (auto &c : tissues)
                    c = {};
            }
            Word before = size(m), prior = active(m);
            work += handle(m, s, law[phase], mix(key + phase * 18041), 2, cond, all);
            switches += bool(before) && prior != active(m);
        }
    }
    if (cond == short_memory)
        m.genes.muscles &= ~(15u << 8);
    auto learned = recall(m, s, 4);
    Word place = mix(key + 10193), actual = (key >> 17) & 1;
    auto rig = apparatus(place, width, law[(key >> 11) & 1]);
    auto score = challenge(m, learned, rig, law[actual], cond, key, grade, spec);
    score.prep = work;
    score.shifts = switches;
    if (spec) {
        spec->seed = key;
        spec->world = place;
        spec->law[0] = worlds[0];
        spec->law[1] = worlds[1];
        spec->chosen = actual;
    }
    return score;
}
} // namespace axion::life::practice
KERNEL void practice_trials(const practice::Genes *g, Word ng, Word n, Word seed, Word size, Word mode, Word cap,
                            practice::Score *out, practice::Specimen *cases) {
    Word i = tid();
    if (i >= n)
        return;
    Word k = mix(seed + i * 6277);
    out[i] = practice::school(g[mix(k + 1009) % ng], k, size, mode, cap, true, i < 4 ? cases + i : nullptr);
}
KERNEL void practice_meals(practice::Family *p, Word n, Word seed, Word epoch, Word cap) {
    Word i = tid();
    if (i >= n)
        return;
    auto &family = p[i];
    auto g = family.genes;
    Word wallet = 28;
    Word k = mix(seed + (i / 2) * 5527 + epoch * 17431), worlds[2];
    linkage::Material material[2][4];
    practice::climate(k, worlds, material);
    practice::Mind m{g};
    practice::Context bank[4]{};
    practice::Slots s{bank, 1, cap};
    for (Word season = 0; season < 3; ++season) {
        Word wet = season & 1;
        practice::handle(m, s, material[wet], mix(k + 1147 * season), 1,
                         (k & 1) ? practice::informed : practice::own_contacts, practice::all);
        auto charts = practice::recall(m, s, 4);
        for (Word task = 0; task < 3; ++task) {
            Word hidden = season > 0 ? (k >> task) & 1 : 0;
            auto request = practice::apparatus(mix(k + task * 8563 + season * 6073), 1 + task, material[hidden]);
            auto trial =
                practice::challenge(m, charts, request, material[hidden], practice::informed, k, false, nullptr);
            wallet += trial.harvest;
            Word fee = 4 + trial.paid;
            wallet -= wallet < fee ? wallet : fee;
        }
    }
    family.eggs = 1 + wallet / 8;
}
KERNEL void practice_breed(const practice::Family *parents, practice::Family *next, Word n, Word seed, Word epoch,
                           Word selected) {
    Word i = tid();
    if (i >= n / 2)
        return;
    Word k = mix(seed + epoch * 17881), stride = mix(k + 5669) | 1;
    Word eggs[]{parents[i * 2].eggs, parents[i * 2 + 1].eggs};
    for (Word j = 0; j < 2; ++j) {
        Word r = mix(k + i * 3203 + j * 2557);
        Word p = selected ? Word(r % (eggs[0] + eggs[1]) >= eggs[0]) : r & 1;
        if (selected)
            --eggs[p];
        auto genes = parents[i * 2 + p].genes, other = parents[i * 2 + 1 - p].genes;
        if ((r >> 8 & 7) == 0) {
            Word mask = ~0u << ((r >> 14) & 31);
            switch ((r >> 11) & 3) {
            case 0:
                genes.inference = (genes.inference & mask) | (other.inference & ~mask);
                break;
            case 1:
                genes.learning = (genes.learning & mask) | (other.learning & ~mask);
                break;
            case 2:
                genes.attention = (genes.attention & mask) | (other.attention & ~mask);
                break;
            default:
                genes.muscles = (genes.muscles & mask) | (other.muscles & ~mask);
                break;
            }
        }
        next[(k + (2 * i + j) * stride) & (n - 1)] = {
            practice::child(genes, mix(r + 1201), 3 + ((genes.inference >> 25) & 31)), 0};
    }
}
KERNEL void practice_candidates(const practice::Genes *g, Word ng, Word seed, Word cap, Word *out) {
    Word i = tid();
    if (i >= ng)
        return;
    Word currency = 0;
    for (Word j = 0; j < 32; ++j) {
        auto r = practice::school(g[i], mix(seed + j * 6659), 2 + j % 3, practice::informed, cap, false, nullptr);
        currency += r.delivered ? 40 - r.paid : 0;
    }
    out[i] = currency;
}
