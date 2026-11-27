namespace axion::life::linkage {
struct Site {
    Rig rig{};
    Word pos[4]{}, slots[4]{}, angle = 0, from = 0, to = 0;
    Pattern pore{};
    Word can_flow = 0, sockets = 0, bands = 0, hydrated = 0;
};
AX_FN void mark_path(Pattern &p, Word mask) {
    Word care = mask_parts(mask);
    if (!p.care)
        p.care = care;
    else if ((p.care & care) != p.care)
        p.alternate = care;
}
AX_FN void geometry(Site &s) {
    Word a = s.from, b = s.to;
    if (a == b || (s.rig.parts & ((1u << a) | (1u << b))) != ((1u << a) | (1u << b)))
        return;
    Word endpoints = (1u << a) | (1u << b);
    auto edges = s.rig.joints;
    if (joint(edges, a, b))
        mark_path(s.pore, endpoints);
    else {
        for (Word j = 0; j < 4; ++j)
            if (joint(edges, a, j) && joint(edges, j, b))
                mark_path(s.pore, endpoints | (1u << j));
        if (!s.pore.care)
            for (Word j = 0; j < 4; ++j)
                for (Word k = 0; k < 4; ++k)
                    if (j != b && k != a && joint(edges, a, j) && joint(edges, j, k) && joint(edges, k, b))
                        mark_path(s.pore, 15);
    }
    s.can_flow = !!s.pore.care;
}
AX_FN Site bench(flood::World w, flood::Configuration c, Word tile,Word suppressed=linkage::no_edit) {
    Site s{};
    Word moisture = 0;
    Word half = c.side / 2, base = 2 * (tile % half) + 2 * c.side * (tile / half);
    for (Word i = 0; i < 4; ++i) {
        Word pos = base + (i & 1) + c.side * (i >> 1);
        s.pos[i] = pos;
        s.slots[i] = w.slots[pos];
        if (w.linkage.mode & hydration)
            moisture += w.land[pos].water;
        auto g = w.matter.bed[pos];
        if (!g.faces)
            continue;
        s.rig.parts |= 1u << i;
        s.rig.glyphs = replace(s.rig.glyphs, i, material::catalytic_band(g));
        s.angle = replace(s.angle, i, g.marks >> 2);
        auto soil = w.land[pos];
        s.pore.faces = replace(s.pore.faces, i, (soil.spore ^ (soil.root >> 11) ^ (soil.enzyme >> 1)) & 3);
    }
    if (!s.rig.parts)
        return s;
    // A spring is fixed by the underlying geology among the present pieces.
    // Adjoining slopes and fibrous bands determine the visible coupling.
    Word k = mix(c.seed + tile * 4093);
    s.hydrated = (w.linkage.mode & hydration) && moisture >= 4 * (24 + ((k >> 9) & 31));
    s.from = bump(s.rig.parts, k);
    s.to = s.from;
    int best = -1;
    for (Word i = 0; i < 4; ++i)
        if (s.rig.parts & (1u << i)) {
            int d = (__builtin_popcount(i ^ s.from) * 4) + int((k + i * 3) & 3);
            if (d > best) {
                best = d;
                s.to = i;
            }
            for (Word axis = 0; axis < 2; ++axis) {
                Word j = i ^ (1u << axis);
                if (i >= j || !(s.rig.parts & (1u << j)))
                    continue;
                auto ground = w.land[s.pos[i]];
                Word bank = axis ? ground.south : ground.east, fibre = axis ? ground.fibre_south : ground.fibre_east;
                Word combined = material::weight(w.matter.bed[s.pos[i]]) + material::weight(w.matter.bed[s.pos[j]]);
                Word socket = 0;
                while (edge_a(socket) != i || edge_b(socket) != j)
                    ++socket;
                if(socket==suppressed && fibre)--fibre;
                if(bank>combined+1+fibre)continue;
                s.sockets |= 1u << socket;
                if (fibre)
                    s.bands |= 1u << socket;
                Word bare = (bank <= combined &&
                             ((combined + bank + material::catalytic_band(w.matter.bed[s.pos[j]])) % 3) != 0)
                                ? 1u
                                : 0u;
                if (w.linkage.mode & coupled)
                    join(s.rig, i, j,
                         fibre ? thread(w.linkage.layers[4 * s.hydrated + bits(s.rig.glyphs, i)], bare) : bare);
            }
        }
    geometry(s);
    return s;
}
AX_FN Word graspable(const Site &s, Word which, flood::Anatomy body) {
    if (!body.limbs)
        return 0;
    Word knobs = 0;
    for (Word i = 0; i < 4; ++i)
        if (s.rig.parts & (1u << i)) {
            Word tax = __builtin_popcount(i ^ which);
            if (tax == 0 || (tax == 1 && __builtin_popcount(body.limbs) > 1))
                knobs |= 3u << (2 * i);
        }
    return knobs;
}
AX_FN Word reachable_edges(const Site &s, Word who, const flood::Anatomy &body) {
    Word v = 0;
    for (Word mask = s.sockets; mask; mask &= mask - 1) {
        Word e = __builtin_ctz(mask);
        Word a = edge_a(e), b = edge_b(e);
        if (a == who || b == who || __builtin_popcount(body.limbs) >= 4)
            v |= 1u << e;
    }
    return v;
}
AX_FN Design water_order(const Mind &mind, const Map &map, Site s, Word colors, Word sockets, Word attention) {
    Design best{};
    if (s.can_flow)
        best = solve_fixed(map, s.rig, colors, s.pore, mind.genes, attention);
    if (best.found && best.effort <= 2)
        return best;
    for (Word bits = sockets; bits; bits &= bits - 1) {
        Word edge = __builtin_ctz(bits);
        auto candidate = s;
        if (!model_splice(map, candidate.rig, edge))
            continue;
        candidate.pore.care = candidate.pore.alternate = candidate.can_flow = 0;
        geometry(candidate);
        if (candidate.can_flow)
            consider(best, attend(map, candidate.rig, colors, candidate.pore, mind.genes, attention), edge);
    }
    return best;
}
AX_FN Word fingerprint(const Site &s, Word exposed) {
    return (exposed & 255) | (s.rig.glyphs << 8) | (s.pore.faces << 16) | (s.rig.parts << 24) | (s.from << 28) |
           (s.to << 30);
}
} // namespace axion::life::linkage
#include "flood_practice.cuh"
KERNEL void linkage_found(linkage::Field f, Word n, Word seed) {
    Word i = tid();
    if (i >= n)
        return;
    if (i < 4) {
        auto dry = linkage::matter(f.seed, i);
        f.layers[i] = dry;
        if (f.mode & linkage::hydration) {
            auto wet = linkage::matter(mix(f.seed + 22679), i);
            wet.dye = dry.dye;
            f.layers[i + 4] = wet;
        }
    }
    f.minds[i] = {linkage::founder(mix(seed + i * 2437))};
    for (Word j = 0; j < f.arena; ++j)
        f.cells[Count(j) * n + i] = {};
    if (i < n / 4)
        f.works[i] = {};
}
KERNEL void linkage_widen(linkage::Field f, const linkage::Provenance *older, Word side, Word old) {
    Word i = tid(), width = old / 2;
    if (i < width * width)
        f.works[(i / width) * (side / 2) + i % width] = older[i];
}
KERNEL void linkage_shop(World w, Configuration c, const Word *clock, Word offset) {
    using namespace linkage;
    Word tile = tid(), tick = *clock + offset;
    if (tile >= c.side * c.side / 4)
        return;
    auto f = w.linkage;
    Word k = mix(c.seed + tile * 1741 + tick * 9091);
    // Spread due blocks evenly across ticks and avoid launching an entire
    // population's private search at once.
    bool due=((tick+tile)&(f.period-1))==0;
    if(!due && (!w.practice.minds || !(w.practice.mode&practice::holding)))return;
    if(!due){
        Word half=c.side/2,pos=2*(tile%half)+2*c.side*(tile/half);
        bool occupied=false;for(Word i=0;i<4;++i){
            auto id=w.slots[pos+(i&1)+(i>>1)*c.side];occupied|=practice::stays(w.practice.minds[id],tile);
        }
        if(!occupied)return;
    }
    if (c.side >= 128) {
        w.account += tile / 128;
        w.hollows.ledger += tile / 128;
        w.matter.ledger += tile / 128;
        f.book += tile / 128;
        if(w.practice.book){w.practice.book+=tile/128;w.practice.credits+=tile/128;}
    }
    if(w.wicks.ledger&&c.side>=128)w.wicks.ledger+=tile/128;
    auto s = bench(w, c, tile);
    auto &record = f.works[tile];
    if (!s.rig.parts) {
        if(w.practice.minds)for(Word i=0;i<4;++i){auto &m=w.practice.minds[s.slots[i]];if(practice::stays(m,tile))m.focus=0;}
        record = {};
        return;
    }
    const Material *law = f.layers + 4 * s.hydrated;
    Word colors = expose(s.rig, s.angle, law);
    bool open = s.can_flow && matches(colors, s.pore);
    if (record.maker && (!open || record.stamp != s.rig.joints || record.pattern != fingerprint(s, colors)))
        record = {};
    auto work=practice::approach(w,s,tile,k,due);
    if(!due && work.owner==4)return;
    if(w.practice.book && due)count(w.practice.book->invitations,1);
    Word who = 4;
    for (Word j = 0; j < 4; ++j) {
        Word choose = (j + (k >> 8)) & 3;
        const auto &a = w.animals[s.slots[choose]];
        if (a.energy >= 4 && a.body.limbs && a.body.eyes) {
            who = choose;
            break;
        }
    }
    if(work.owner<4)who=work.owner;
    bool engaged=work.owner<4;
    Word practice_mode=c.plastic?w.practice.mode:w.practice.mode&~Word(practice::studying);
    if (who < 4 && (f.mode & working)) {
        Word id = s.slots[who];
        auto &actor = w.animals[id];
        auto &mind = f.minds[id];
        auto tissue = cells(f, id);
        Word motive = (w.motor[id] & 7);
        s.rig.handles = graspable(s, who, actor.body);
        if(s.rig.handles && (engaged || ((mind.genes.impulse & (1u << (16 + motive))) && !((k >> 22) & 1)))) {
            count(f.book->active, 1);
            Word motor = no_path, edit = no_edit;
            bool intended = false, adding = true;
            Word edges =
                (c.weaving && (f.mode & fitting) && (f.mode & coupled)) ? reachable_edges(s, who, actor.body) : 0;
            bool inherited = public_motor(f.mode, w.hollows.minds[id].efferent & hollow::lineage_tag);
            bool energy = w.hollows.beds[s.pos[s.from]].carbon != 0;
            if(engaged){
                practice::act(w,c,s,work,colors,tile,k,open);
                motor=work.stroke;edit=work.edge;adding=work.adding;intended=work.designed;
                if(work.designed){count(f.book->attempts,1);count(f.book->invented,1);if(edit!=no_edit){count(f.book->designs,1);count(w.practice.book->fittings,1);}}
                else if(work.exploratory)count(f.book->explore,1);
            }
            if (!engaged && !open && energy && inherited && (f.mode & planning) && mind.tags && ((k >> 10) & 3) == 0) {
                auto chart = compile(mind, tissue);
                if (f.mode & scrambled)
                    for (Word g = 0; g < 4; ++g) {
                        Word tmp = chart.link[0][g];
                        chart.link[0][g] = chart.link[1][g];
                        chart.link[1][g] = tmp;
                    }
                Word afford =
                    (actor.fibre && (f.mode & designing) && (mind.genes.impulse & (1u << 30))) ? edges & ~s.bands : 0;
                auto plan = water_order(mind, chart, s, colors, afford, f.mode & focal);
                count(f.book->attempts, 1);
                if (plan.found && (plan.turns.len || plan.edit != no_edit)) {
                    intended = true;
                    count(f.book->invented, 1);
                    if (plan.edit != no_edit) {
                        edit = plan.edit;
                        count(f.book->designs, 1);
                    } else
                        motor = step(plan.turns, 0);
                }
            }
            if (!engaged && motor == no_path && edit == no_edit && (!open || !(mind.genes.arbor & (1u << 18))) &&
                ((k >> 14) & 15) <= ((mind.genes.impulse >> 24) & 7)) {
                Word possible = actor.fibre ? edges & ~s.bands : edges & s.bands;
                if (possible && (k & 7) <= ((mind.genes.arbor >> 28) & 3)) {
                    edit = bump(possible, mix(k + 7207));
                    adding = actor.fibre != 0;
                } else
                    motor = probe_motor(mind, tissue, s.rig, colors, k);
                count(f.book->explore, 1);
            }
            if (edit != no_edit) {
                Word a = edge_a(edit), b = edge_b(edit), old = joint(s.rig.joints, a, b);
                auto &bed = w.land[s.pos[a]];
                auto &cord = (a ^ b) == 1 ? bed.fibre_east : bed.fibre_south;
                if ((adding && actor.fibre && cord < 65535) || (!adding && cord && actor.fibre < 65535)) {
                    if(w.practice.marks){
                        auto &mark=w.practice.marks[tile];Word edge=a|(b<<2);
                        if(mark.edge==edge)mark.who=0;
                        if(engaged&&intended&&adding&&!cord){mark={id+1,actor.born,tick,edge};count(w.practice.credits->installed,1);}
                    }
                    if (adding) {
                        --actor.fibre;
                        ++cord;
                        count(w.account->fibre_laid, 1);
                        count(f.book->loops_in, 1);
                    } else {
                        --cord;
                        ++actor.fibre;
                        count(w.account->fibre_lifted, 1);
                        count(f.book->loops_out, 1);
                    }
                    tag_strand(w,c.side,s.pos[a],s.pos[b],adding?cord-1:cord+1,cord);
                    Word fee = adding ? 2u : 1u;
                    actor.energy -= static_cast<Half>(fee);
                    count(w.account->heat, fee);
                    count(f.book->heat, fee);
                    s = bench(w, c, tile);
                    Word next = joint(s.rig.joints, a, b), glyph = bits(s.rig.glyphs, a);
                    Word flags = c.plastic ? f.mode : f.mode & ~Word(writable);
                    if(w.practice.minds){
                        if(engaged){count(adding?w.practice.book->placed:w.practice.book->removed,1);count(w.practice.book->work,fee);}
                        if(adding)for(Word j=0;j<4;++j){Word other=s.slots[j];if(!w.animals[other].energy||!w.animals[other].body.eyes)continue;
                            count(w.practice.book->loops_learned,practice::fitting(w.practice.minds[other],practice::cells(w.practice,other),glyph,old,next,j!=who,practice_mode));
                        }
                    }
                    count(f.book->join_edits, learn_join(mind, tissue, glyph, old, adding, next, false, flags));
                    for (Word j = 0; j < 4; ++j)
                        if (j != who && (f.mode & witnesses)) {
                            auto &guest = w.animals[s.slots[j]];
                            auto &brain = f.minds[s.slots[j]];
                            if (guest.energy && guest.body.eyes &&
                                (brain.genes.arbor & (1u << (20 + (w.motor[s.slots[j]] & 7)))))
                                count(f.book->join_edits,
                                      learn_join(brain, cells(f, s.slots[j]), glyph, old, adding, next, true, flags));
                        }
                    bool aligned = s.can_flow && matches(colors, s.pore);
                    if (!open && aligned) {
                        record = {id + 1, actor.born, tick, s.rig.joints, fingerprint(s, colors)};
                        count(f.book->stamped, 1);
                        count(f.book->solved, intended);
                        count(f.book->certified_joins, 1);
                        if(engaged&&intended&&adding&&w.practice.credits)count(w.practice.credits->immediate,1);
                        if(engaged&&intended)count(w.practice.book->alignments,1);
                    } else if (!aligned || old != next)
                        record = {};
                    open = aligned;
                }
            }
            if (motor != no_path) {
                auto touched=s.rig;if(work.braced)touched.joints=0;
                Word next = linkage::press(touched, s.angle, motor, law), after = expose(s.rig, next, law), moved = 0;
                for (Word j = 0; j < 4; ++j) {
                    Word rotation = (bits(next, j) - bits(s.angle, j)) & 3;
                    if (rotation) {
                        material::rotate_grain(w.matter.bed[s.pos[j]], rotation);
                        ++moved;
                    }
                }
                --actor.energy;
                count(w.account->heat, 1);
                count(f.book->heat, 1);
                count(f.book->taps, 1);
                count(w.matter.ledger->turns, moved);
                count(f.book->transmitted, moved - 1);
                Word flags = c.plastic ? f.mode : f.mode & ~Word(writable);
                Obs saw{touched, colors};
                if(w.practice.minds){
                    if(engaged){
                        if(work.braced){--actor.energy;count(w.account->heat,1);}
                        count(w.practice.book->work,1+work.braced);count(w.practice.book->strokes,1);
                        count(w.practice.book->direct_known,work.prediction!=absent);count(w.practice.book->direct_exact,work.prediction==after);
                    }
                    practice::record(w,s,who,touched,colors,motor,after,practice_mode);
                }
                count(f.book->self_changes, learn(mind, tissue, saw, motor, after, false, flags));
                if (f.mode & witnesses)
                    for (Word j = 0; j < 4; ++j)
                        if (j != who) {
                            auto &v = w.animals[s.slots[j]];
                            if (!v.energy || !v.body.eyes)
                                continue;
                            auto &guest = f.minds[s.slots[j]];
                            Word watch = (guest.genes.arbor >> (20 + (w.motor[s.slots[j]] & 7))) & 1;
                            if (watch) {
                                count(f.book->seen, 1);
                                count(f.book->public_changes,
                                      learn(guest, cells(f, s.slots[j]), saw, motor, after, true, flags));
                            }
                        }
                bool aligned = s.can_flow && matches(after, s.pore);
                if (!open && aligned) {
                    record = {id + 1, actor.born, tick, s.rig.joints, fingerprint(s, after)};
                    if(engaged&&intended)count(w.practice.book->alignments,1);
                    count(f.book->stamped, 1);
                    count(f.book->solved, intended);
                } else if (!aligned || colors != after)
                    record = {};
                open = aligned;
                colors = after;
            }
        }
    }
    if (!due || !open || !(f.mode & current))
        return;
    auto &a = w.land[s.pos[s.from]], &b = w.land[s.pos[s.to]];
    auto &source = w.hollows.beds[s.pos[s.from]];
    bool necessary=false;
    practice::Mark made{};
    if(w.practice.marks && (made=w.practice.marks[tile]).who){
        Word edge=0;while(edge<6 && (edge_a(edge)|(edge_b(edge)<<2))!=made.edge)++edge;
        if(edge<6 && (s.bands&(1u<<edge))){count(w.practice.credits->channel_open,1);
            auto without=bench(w,c,tile,edge);necessary=!without.can_flow || !matches(colors,without.pore);
            count(w.practice.credits->necessary,necessary);
        }
    }
    if (a.water > b.water + 1) {
        Word drip = pour(a.water, b.water, 1 + (a.water - b.water) / 16);
        count(w.account->flow, drip);
        count(f.book->water, drip);if(necessary)count(w.practice.credits->water,drip);
    }
    if (!source.carbon || !a.water)
        return;
    auto &receiver = w.animals[s.slots[s.to]];
    Word amount = 1 + cut(5, a.water / 24), food = 0;
    if (receiver.energy && receiver.body.pores) {
        food = pour(source.carbon, receiver.energy, amount);
        if(food&&w.practice.minds)practice::reinforce(w.practice.minds[s.slots[s.to]],false,false,true,c.plastic?w.practice.mode:w.practice.mode&~Word(practice::studying));
        receiver.meals += food;
        receiver.diet[1] += food;
        count(w.account->harvest, food);
        count(w.account->digested, food);
        count(w.hollows.ledger->eaten, food);
    }
    Word loose = pour(source.carbon, b.fruit, amount - food);
    count(w.hollows.ledger->released, food + loose);
    count(f.book->flow, food + loose);
    count(f.book->eaten, food);
    count(f.book->loose, loose);
    if(necessary){
        auto &book=*w.practice.credits;count(book.transport,food+loose);count(book.eaten,food);
        bool other=made.who!=s.slots[s.to]+1||made.born!=receiver.born;
        if(other&&food){count(book.other_events,1);count(book.other_food,food);
            if(made.date==~0u||made.date<receiver.born){count(book.older_events,1);count(book.older_food,food);}}
    }
    if (food && record.maker) {
        bool other = record.maker != s.slots[s.to] + 1 || record.born != receiver.born;
        bool older = record.dated == ~0u || record.dated < receiver.born;
        count(f.book->own_meal, !other);
        count(f.book->other_meal, other);
        count(f.book->older_meal, other && older);
        count(f.book->other_carbon, other ? food : 0);
        count(f.book->older_carbon, other && older ? food : 0);
    }
}
KERNEL void linkage_disturb(World w, Configuration c, Word n) {
    Word tile = tid();
    if (tile >= n)
        return;
    auto s = linkage::bench(w, c, tile);
    auto &mark = w.linkage.works[tile];
    if (mark.maker && s.rig.parts) {
        Word pos = s.pos[s.from];
        material::rotate_grain(w.matter.bed[pos], 1);
        mark = {};
    }
}
#include "axion/linkage_image.hpp"
KERNEL void linkage_views(World w, Configuration c, Word seed, Word tick, Word treatment, Word width,
                          linkage::Panel *panels, linkage::Audit *partial) {
    using namespace linkage;
    Word tile = tid();
    if (tile >= c.side * c.side / 4)
        return;
    Audit r{};
    Panel panel{};
    auto s = bench(w, c, tile);
    Word k = mix(seed + tile * 2791), who = 4;
    const auto *law = w.linkage.layers + 4 * s.hydrated;
    r.hydrated = s.hydrated;
    r.grains = __builtin_popcount(s.rig.parts);
    for (Word j = 0; j < 4; ++j) {
        Word ix = (j + k) & 3;
        auto &a = w.animals[s.slots[ix]];
        if (a.energy) {
            ++r.residents;
            if (who == 4 && a.body.limbs && a.body.eyes)
                who = ix;
        }
    }
    Word colors = expose(s.rig, s.angle, law);
    r.wet = s.can_flow && w.hollows.beds[s.pos[s.from]].carbon;
    r.ready = r.wet && matches(colors, s.pore);
    auto m = w.linkage.works[tile];
    r.marked = !!m.maker;
    r.old = m.maker && (m.dated == ~0u || tick - m.dated >= 16);
    if (who < 4 && s.rig.parts) {
        Word id = s.slots[who];
        auto &body = w.linkage.minds[id];
        auto chart = compile(body, cells(w.linkage, id));
        s.rig.handles = graspable(s, who, w.animals[id].body);
        if (treatment == 1)
            for (Word g = 0; g < 4; ++g)
                chart.link[0][g] = chart.link[1][g] = 0;
        if (treatment == 2)
            for (Word g = 0; g < 4; ++g) {
                Word t = chart.link[0][g];
                chart.link[0][g] = chart.link[1][g];
                chart.link[1][g] = t;
            }
        if (treatment == 3) {
            for (Word g = 0; g < 4; ++g)
                chart.next[g] = 0;
        }
        for (Word mask = s.rig.handles; mask; mask &= mask - 1) {
            Word a = __builtin_ctz(mask), achieved = expose(s.rig, linkage::press(s.rig, s.angle, a, law), law),
                 p = predict(chart, s.rig, colors, a);
            ++r.forecasts;
            r.known += p != absent;
            r.correct += p == achieved;
        }
        if (r.wet && !r.ready) {
            r.purpose = 1;
            auto p = attend(chart, s.rig, colors, s.pore, body.genes, w.linkage.mode & focal);
            r.requested = p.found;
            Word pose = s.angle;
            for (Word j = 0; j < p.course.len; ++j)
                pose = linkage::press(s.rig, pose, step(p.course, j), law);
            r.productive = p.found && matches(expose(s.rig, pose, law), s.pore);
            r.motion = p.course.len;
            r.indirect = r.productive && p.course.len > 1;
        }
        if (panels && s.can_flow && __builtin_popcount(s.rig.parts) == width && s.rig.handles && body.tags) {
            Word remap[4]{}, at = 0;
            panel.model = chart;
            panel.body = body.genes;
            panel.owner = id;
            for (Word j = 0; j < 4; ++j)
                if (s.rig.parts & (1u << j)) {
                    remap[j] = at;
                    panel.rig.parts |= 1u << at;
                    panel.rig.glyphs = replace(panel.rig.glyphs, at, bits(s.rig.glyphs, j));
                    panel.rig.handles |= bits(s.rig.handles, j) << (2 * at);
                    panel.target.faces = replace(panel.target.faces, at, bits(s.pore.faces, j));
                    panel.target.care = replace(panel.target.care, at, bits(s.pore.care, j));
                    panel.target.alternate = replace(panel.target.alternate, at, bits(s.pore.alternate, j));
                    panel.view = replace(panel.view, at, bits(colors, j));
                    ++at;
                }
            for (Word i = 0; i < 4; ++i)
                for (Word j = i + 1; j < 4; ++j)
                    if (Word c = joint(s.rig.joints, i, j))
                        join(panel.rig, remap[i], remap[j], c);
        }
    }
    if (panels)
        panels[tile] = panel;
    if (partial)
        partial[tile] = r;
}
KERNEL void practice_unthread(World w,Configuration c,Word count_tiles){
    Word tile=tid();if(tile>=count_tiles)return;
    auto &r=w.practice.marks[tile];if(!r.who)return;
    Word owner=r.edge&3,other=r.edge>>2;
    Word half=c.side/2,base=2*(tile%half)+2*c.side*(tile/half),pos=base+(owner&1)+(owner>>1)*c.side;
    auto &soil=w.land[pos];auto &strand=(owner^other)==1?soil.fibre_east:soil.fibre_south;
    if(strand==1 && soil.litter<65535){--strand;++soil.litter;
        drift::disturb(w.drift_marks,c.side,pos,(owner^other)!=1);
        if(w.wicks.reeds.seams)runnel::set_fold(w.wicks.reeds.seams[pos],(owner^other)!=1,0);if(w.wicks.parcels){w.wicks.parcels[pos]&=~wicks::edge_bit((owner^other)!=1);auto *audit=w.wicks.ledger+(c.side>=128?tile/128:0);count(audit->frayed,1);}auto *ledger=w.practice.credits+(c.side>=128?tile/128:0);
        count(ledger->stripped,1);auto *total=w.account+(c.side>=128?tile/128:0);count(total->frayed,1);r={};w.linkage.works[tile]={};}
    else if(strand!=1)r={};
}
KERNEL void practice_release_attention(practice::Field f){Word i=tid();if(i<f.sites)f.minds[i].focus=0;}
