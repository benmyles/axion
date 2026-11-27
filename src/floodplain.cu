#include "practice_cells.cuh"
#include "linkage_design.cuh"
#include "linkage_physics.cuh"
#include "linkage_cells.cuh"
#include "effort_bundle.cuh"
#include "hollow_contest.hpp"
#include "grounding_eye.cuh"
#include "flood_body.cuh"
#include "flood_wicks.cuh"
#include "flood_runnels.cuh"
#include "flood_water.cuh"
#include "flood_rattles.cuh"
#include "flood_braids.cuh"
#include "flood_braid_gates.cuh"
#include "flood_pods.cuh"
#include "flood_material.cuh"
#include "flood_foresight.cuh"
#include "flood_tray.hpp"
#include "axion/gate_trial.hpp"
#include "axion/shore.hpp"
#include "axion/drift.hpp"
#include "axion/retina.hpp"
#include "axion/capillary_field.hpp"
using namespace axion;
using namespace axion::life;
using namespace axion::life::flood;
#define KERNEL extern "C" __attribute__((global))
#define UNROLL _Pragma("unroll")
namespace {
AX_FN Word tid() {
    Word t, b, n;
    asm("mov.u32 %0, %%tid.x;" : "=r"(t));
    asm("mov.u32 %0, %%ctaid.x;" : "=r"(b));
    asm("mov.u32 %0, %%ntid.x;" : "=r"(n));
    return b * n + t;
}
AX_FN Word shuffle(Word x, Word source, Word mask) {
    Word v;
    asm volatile("shfl.sync.idx.b32 %0,%1,%2,31,%3;" : "=r"(v) : "r"(x), "r"(source), "r"(mask));
    return v;
}
#include "somatic_genome.cuh"
#include "somatic_core.cuh"
#include "flood_nerve_pages.cuh"
AX_FN Germ egg(Word key) {
    Germ g;
    g.brain = founder(key);
    for (Word j = 0; j < 4; ++j) {
        g.buds[j] = mix(key + j * 8811);
        g.pigments[j] = mix(key + j * 913 + 3321);
    }
    g.growth = mix(key + 751);
    g.grip = mix(key + 43);
    g.stomach = mix(key + 99);
    g.voice = mix(key + 137);
    g.hearing = {mix(key + 517), mix(key + 124), mix(key + 145)};
    g.allocation = mix(key + 559);
    // One founder contact for each mechanical port. Mutations may erase,
    // invert or recruit them in combination with any neural motor population.
    g.brain.effectors[0] = 0x0c050408u;
    g.brain.effectors[1] = 0x80602010u;
    return g;
}
struct Mutation {
    Word edits = 0, tape = 0, body = 0, modules = 0;
};
AX_FN Germ offspring(const Germ &parent, Word key, Configuration c, Mutation &m) {
    Germ child = parent;
    inherit(parent.brain, child.brain, key, true, m.edits, m.tape);
    if (!c.architecture) {
        auto born = child.brain;
        child.brain = parent.brain;
        child.brain.reproduction = born.reproduction;
        child.brain.body = born.body;
        child.brain.effectors[0] = born.effectors[0];
        child.brain.effectors[1] = born.effectors[1];
        m.tape = 0;
        m.edits = Word(born.reproduction != parent.brain.reproduction) + Word(born.body != parent.brain.body) +
                  Word(born.effectors[0] != parent.brain.effectors[0]) +
                  Word(born.effectors[1] != parent.brain.effectors[1]);
    }
    Word probability = 4 + ((parent.brain.reproduction >> 8) & 31);
    auto mutate = [&](Word &v, Word which, bool body) {
        Word pick = mix(key + which * 0xd6793f2du);
        if ((pick & 255) < probability) {
            v ^= 1u << ((pick >> 10) & 31);
            ++m.edits;
            m.body += body;
        }
    };
    if (c.architecture) {
        mutate(child.allocation, 27, false);
        for (Word j = 0; j < child.module_count; ++j) {
            mutate(child.module[j].routing, 28 + 2 * j, false);
            mutate(child.module[j].gate, 29 + 2 * j, false);
        }
        Word roll = mix(key ^ 0x811059d5u), n = child.module_count;
        if ((roll & 255) < 2 + ((parent.brain.expression >> 11) & 7)) {
            Word from = (roll >> 12) % n;
            if (n > 1 && (roll & (1u << 25))) {
                child.module[from] = child.module[n - 1];
                child.module[n - 1] = {};
                --child.module_count;
                ++m.modules;
            } else if (n < 4) {
                child.module[n] = child.module[from];
                ++child.module_count;
                ++m.modules;
            }
            m.edits += m.modules;
        }
    }
    if (c.morph) {
        for (Word j = 0; j < 4; ++j)
            mutate(child.buds[j], 3 + j, true);
        mutate(child.growth, 9, true);
        mutate(child.grip, 10, true);
        mutate(child.stomach, 11, true);
        Word roll = mix(key ^ 0xaf172e35u), n = 1 + (child.growth & 15);
        if ((roll & 255) < 2 + ((child.growth >> 16) & 7)) {
            Word j = (roll >> 9) % n, at = (roll >> 16) % n;
            Byte gene[16];
            for (Word t = 0; t < 16; ++t)
                gene[t] = static_cast<Byte>(child.buds[t / 4] >> (8 * (t & 3)));
            if ((roll >> 30) & 1 && n > 1) {
                for (Word k = at; k + 1 < n; ++k)
                    gene[k] = gene[k + 1];
                --n;
            } else if (n < 16) {
                Byte duplicate = gene[j];
                for (Word k = n; k > at; --k)
                    gene[k] = gene[k - 1];
                gene[at] = duplicate;
                ++n;
            } else
                gene[at] = gene[j];
            for (Word t = 0; t < 4; ++t)
                child.buds[t] = Word(gene[4 * t]) | (Word(gene[4 * t + 1]) << 8) | (Word(gene[4 * t + 2]) << 16) |
                                (Word(gene[4 * t + 3]) << 24);
            child.growth = (child.growth & ~15u) | (n - 1);
            ++m.body;
            ++m.edits;
        }
    }
    mutate(child.voice, 12, false);
    for (Word j = 0; j < 4; ++j)
        mutate(child.pigments[j], 14 + j, false);
    mutate(child.hearing.low, 22, false);
    mutate(child.hearing.high, 23, false);
    mutate(child.hearing.expression, 24, false);
    return child;
}
#include "flood_neurogenesis.cuh"
AX_FN void newborn(World w, Word slot, const Germ &g, Word family, Word depth, Word tick, Word energy,
                   Configuration c) {
    Colony a;
    a.energy = static_cast<Half>(energy);
    a.founder = family;
    a.generation = depth;
    a.born = tick;
    a.body = grow(g);
    a.heading = static_cast<Byte>(mix(c.seed + tick + slot) & 3);
    a.body_origin = a.brain_origin = family;
    w.animals[slot] = a;
    w.dna[slot] = g;
    w.ears[slot] = {};
    if (w.braids.trails) w.braids.trails[slot]=0;
    if(w.capillaries)w.capillaries[slot]={};
    if(w.material_history)w.material_history[slot]={};
    if(w.chemical_contacts)w.chemical_contacts[slot]=0;
    if(w.grip_contacts)w.grip_contacts[slot]=0;
    if (w.worksites.residents)
        w.worksites.residents[slot] = {mix(c.seed + slot * 211), mix(c.seed + slot * 731)};
    if (w.tide.cells)
        w.tide.cells[slot] = {};
    if (w.brine.animals)
        w.brine.animals[slot].memory = {};
    if(w.practice.minds){
        // Initial residents retain the germline placed by practice_found. An
        // actual birth replaces it with the parent's mutated genes below.
        auto heritage=w.practice.minds[slot].genes;
        w.practice.minds[slot]={heritage};
        for(Word j=0;j<w.practice.cap;++j)w.practice.banks[Count(j)*w.practice.sites+slot]={};
    }
    if(w.linkage.minds){
        w.linkage.minds[slot]={linkage::founder(mix(c.seed+slot*2437))};
        for(Word j=0;j<w.linkage.arena;++j)w.linkage.cells[Count(j)*w.linkage.count+slot]={};
    }
    if(w.outlay.minds) w.outlay.minds[slot]={effort::founder(mix(c.seed+slot*3389))};
    if (w.hollows.minds) {
        auto &f=w.hollows;f.minds[slot]={hollow::founder(mix(c.seed+slot*8929))};
        for(Word j=0;j<f.slots;++j) {
            f.marks[Count(j)*f.stride+slot]=0;
            if(!(slot&15)) f.sites[Count(j)*((f.stride+15)/16)+slot/16]=0;
        }
    }
    if (w.clay.minds)
        w.clay.minds[slot] = {clay::founder(mix(c.seed + slot * 10039))};
    if (w.siphons.residents)
        w.siphons.residents[slot] = {siphon::develop(mix(c.seed + slot * 2539), w.siphons.windows & 32)};
    w.motor[slot] = 0;
    Word active = living_tiles(c.tiles, g.brain.development);
    if (w.cortex.resident)
        w.cortex.resident[slot] = hatch_cortex(growth_founder(mix(c.seed + slot + 19561)), active);
    if (w.neural_pages.index) {
        w.neural_pages.origin[slot] = w.cortex.resident ? active : c.tiles;
        w.neural_pages.overrides[slot] = 0;
    } else
        for (Word j = 0; j < c.tiles; ++j)
            write_band(w, c.tiles, slot, j,
                       !w.cortex.resident || j < active ? developmental_band(g.brain.development, j) : Tissue{});
}
AX_FN void payment(Colony &a, Silt &site, Word fee, Counters &total) {
    Word cost = cut(a.energy, fee);
    a.energy -= static_cast<Half>(cost);
    count(total.heat, cost);
    if (!a.energy) {
        pour(a.load, site.grains, a.load);
        pour(a.fibre, site.litter, a.fibre);
        count(total.deaths, 1);
    }
}
AX_FN void feed(Colony &a, const Germ &g, Silt &soil, Word key, Counters &total) {
    Word pores = 1 + nbits(a.body.pores), fit = 8 - nbits((soil.spore ^ g.stomach) & 255),
         mouth = 2 + pores + ((g.grip >> 7) & 3);
    Half *menu[] = {&soil.leaf, &soil.fruit, &soil.litter};
    Word mode = (g.stomach >> 9) & 3;
    Word food = mode == 3 ? (soil.fruit > soil.leaf / 3 ? 1 : 0) : mode;
    if (!*menu[food] && (g.stomach & (1u << 15)))
        food = (food + 1 + (key & 1)) % 3;
    Word bite = cut(*menu[food], mouth), efficiency = food == 0   ? fit + 3
                                                      : food == 1 ? 10
                                                                  : 1 + ((g.stomach >> 17) & 7);
    Word useful = cut(65535u - a.energy, bite * efficiency / 12), waste = cut(bite - useful, 65535u - soil.litter);
    if (food == 2) {
        waste = 0;
        bite = useful;
    } else
        bite = useful + waste;
    *menu[food] -= static_cast<Half>(bite);
    a.energy += static_cast<Half>(useful);
    soil.litter += static_cast<Half>(waste);
    a.meals += useful;
    a.diet[food] += useful;
    count(total.harvest, bite);
    count(total.digested, useful);
}
AX_FN void regurgitate(Word mass, Silt &soil, Counters &budget) {
    Word stored = cut(mass, 65535u - soil.litter);
    soil.litter += static_cast<Half>(stored);
    count(budget.heat, mass - stored);
}
AX_FN void settle_crop(pods::Animal &organ, Colony &a, Silt &soil, Word tick, Word treatment, Word sensory,
                       Counters &total, pods::Census &ledger) {
    auto receipt = pods::mature(organ.gut, tick);
    Word mass = receipt.edible + receipt.waste;
    if (!mass) {
        if (treatment && (sensory & 16))
            pods::omission(organ.brain, organ.genes);
        return;
    }
    Word absorbed = cut(receipt.edible, 65535u - a.energy);
    a.energy += static_cast<Half>(absorbed);
    a.meals += absorbed;
    Word left = absorbed;
    for (Word k = 0; k < 3; ++k) {
        Word taken = cut(left, receipt.food[k]);
        a.diet[k] += taken;
        left -= taken;
    }
    regurgitate(mass - absorbed, soil, total);
    count(total.digested, absorbed);
    count(ledger.mature, 1);
    count(ledger.returned, absorbed);
    count(ledger.waste, mass - absorbed);
    if ((sensory & 4) && treatment)
        pods::reframe(organ.frame, organ.frame_genes, organ.genes, organ.brain, absorbed != 0, tick, treatment == 2);
    count(ledger.edges, pods::associate(organ.brain, organ.genes, absorbed != 0, treatment, tick));
}
AX_FN void empty_crop(pods::Animal &organ, Silt &soil, Counters &total, pods::Census &stats) {
    Word mass = pods::content(organ.gut);
    organ.gut = {};
    regurgitate(mass, soil, total);
    count(stats.waste, mass);
}
AX_FN Word pod_reach(Word pos, const Colony &a, const Germ &g, World w, Configuration c, Word tick) {
    Word axis = ((tick + 2) >> 1) & 1, coordinate = axis ? pos / c.side : pos % c.side;
    Word dir = (axis ? 2 : 1) ^ (((coordinate ^ ((tick + 2) & 1)) & 1) << 1);
    Word reach = segment(a.body.pores, dir);
    if (!(g.grip & 1) || a.heading != dir || !reach)
        return pos;
    Word facing = neighbor(pos, dir, c.side);
    auto s = w.land[pos], f = w.land[facing];
    Word wall = dir == 1 ? s.east : dir == 2 ? s.south : dir == 3 ? f.east : f.south;
    return wall < 1 + nbits(a.body.limbs) + reach ? facing : pos;
}
AX_FN bool pod_bite(Colony &a, const Germ &g, Silt &soil, Word site, World w, Configuration c, Word packed, Word tick,
                    Word key, Word id) {
    auto &p = w.pods;
    auto &crop = p.animal[id];
    auto &shell = p.skin[site];
    auto &stats = *p.ledger;
    Word food = (g.stomach >> 9) & 3;
    if (food == 3)
        food = soil.fruit > soil.leaf / 3 ? 1 : 0;
    Half *menu[] = {&soil.leaf, &soil.fruit, &soil.litter};
    if (!*menu[food] && (g.stomach & (1u << 15)))
        food = (food + 1 + (key & 1)) % 3;
    if ((p.physics.background & 1) && food == 2) {
        feed(a, g, soil, key, *w.account);
        return false;
    }
    Word mass = cut(*menu[food], cut(15, 3 + nbits(a.body.pores) + ((g.grip >> 7) & 3)));
    if (!mass)
        return false;
    Word light = pods::light(soil.spore, shell.winding, optical(soil, a.energy));
    Word district = (site % c.side) / 8 + 128 * ((site / c.side) / 8);
    Word climate = pods::key_for(c.seed, district, tick, p.physics), grasp = (packed >> 3) & 3;
    bool opened = grasp == pods::aperture(light, climate);
    Word lag = pods::latency(light, climate, p.physics), due = (tick + lag) & 7;
    bool occupied = crop.gut.food[0][due] || crop.gut.food[1][due] || crop.gut.food[2][due] || crop.gut.chaff[due];
    *menu[food] -= static_cast<Half>(mass);
    Word calories = opened ? cut(mass, 1 + mass * (7 + ((g.stomach >> 18) & 7)) / 15) : 0;
    pods::swallow(crop.gut, tick, lag, food, mass, calories);
    shell.winding = static_cast<Byte>((shell.winding + 1 + (grasp >> 1)) & 3);
    count(w.account->harvest, mass);
    count(stats.bites, 1);
    count(stats.opened, opened);
    count(stats.swallowed, mass);
    count(stats.overlaps, occupied);
    return true;
}
#include "flood_clay_world.cuh"
#include "flood_effort.cuh"
#include "flood_hollows.cuh"
#include "flood_press.cuh"
#include "flood_material_world.cuh"
#include "flood_siphon_world.cuh"
AX_FN void act(Colony &a, Colony &other, const Germ &g, Silt &site, Silt &facing, Half &wall, Half &weave, bool aligned,
               Word packed, Word key, Configuration c, Word tick, World w, Word id, Word pos, Word across,
               Word edge_direction) {
    Word before_meal = a.meals;
    auto &total = *w.account;
    Word action = packed & 7;
    Word port = effector(g.brain, action), size = nbits(a.body.shape);
    Word fee = 1 + Word((key & 15) < size / 3) + Word(((key >> 4) & 15) < expressed_cells(w, id, g.brain, c.tiles));
    Word reach = segment(a.body.pores, a.heading), hook = 1 + nbits(a.body.limbs);
    a.wet_time += site.water >= 32;
    a.action = static_cast<Byte>(action);
    a.signal = 0;
    // Roots feed locally; projecting appendages may feed across the face.
    auto &target = aligned && reach && (g.grip & 1) && wall < hook + reach ? facing : site;
    bool grasped = false;
    if (port & 8) {
        bool accessible = true;
        if (w.bottom) {
            // Water depth limits a bottom-feeding pore. Mineral sediment and
            // attached clay or fibre raise the bed within reach of the animal.
            Word reach_depth = 4 + 2 * nbits(a.body.pores) + 2 * reach;
            Word floor = cut(12u, Word(target.grains) >> 1);
            if (w.shoals) {
                Word at = &target == &site ? pos : across;
                Word outcrop = 2 * terrain_height(at, c), falling = Word(tide(at, tick, c)) / 4;
                floor += outcrop > falling ? outcrop - falling : 0;
            }
            Word works =
                cut(12u, Word(target.east) + target.south) + cut(8u, Word(target.fibre_east) + target.fibre_south);
            accessible = target.water <= reach_depth + floor + works;
            auto &record = w.bottom[(pos >> 5) & 127];
            count(record.attempted, 1);
            count(record.obstructed, !accessible);
            count(record.supported, accessible && target.water > reach_depth + floor);
        }
        if (accessible) {
            if (w.pods.animal) {
                auto &organ = w.pods.animal[id];
                bool active =
                    !(w.pods.physics.background & 2) ||
                    pods::strike(organ.brain, organ.genes, packed >> 5, (packed >> 3) & 3, pods::occupied(organ.gut));
                if (active)
                    grasped = pod_bite(a, g, target, &target == &site ? pos : across, w, c, packed, tick, key, id);
                else
                    count(w.pods.ledger->withheld, 1);
            } else
                feed(a, g, target, key, total);
            if (w.bottom)
                count(w.bottom[(pos >> 5) & 127].immediate_food, a.meals - before_meal);
        }
    }
    if (w.pods.animal) {
        auto &o = w.pods.animal[id];
        Word cue = packed >> 5, muscle = (packed >> 3) & 3;
        if (pods::sensory_channels(w.pods.physics, a.cohort) & 4) {
            Word e = o.emission;
            pods::MotorPulse pulse{cue, e & 3, muscle, (e >> 2) & 3, (e >> 4) & 15, bool(e & 256), bool(e & 512)};
            if (c.plastic && w.pods.physics.credit && (!a.cohort || a.cohort != w.pods.physics.cut_cohort))
                pods::perceive(o.frame, o.frame_genes, pulse, grasped);
            else {
                o.frame.reliable = static_cast<Byte>((o.frame.reliable << 1) | Word(pulse.anchored && grasped));
                o.frame.signatures = (o.frame.signatures << 4) | pulse.predictions;
            }
            muscle = pulse.native;
        }
        pods::remember(o.brain, cue, muscle, grasped);
    }
    // Motor contacts control the grip and release independently. A heritable
    // four-entry table selects the grasped material from local conditions.
    Word choice = Word(target.water >= 32) + 2 * Word(target.litter > target.grains);
    bool organic = c.weaving && ((g.grip >> (24 + choice)) & 1);
    if ((port & 16) && aligned) {
        Word capacity = 2 + 2 * hook, used = a.load + a.fibre, load = 0;
        if (used < capacity) {
            Half &bound = organic ? weave : wall;
            Half &loose = organic ? target.litter : target.grains;
            bool from_edge=bound&&(g.grip&2);auto &source = from_edge ? bound : loose;
            load = pour(source, organic ? a.fibre : a.load, cut(capacity - used, 1 + hook / 3));
            count(organic ? total.fibre_lifted : total.lifted, load);
            if(load&&from_edge&&organic){practice::disturb(w.practice,c.side,pos,across);tag_strand(w,c.side,pos,across,weave+load,weave);}
        }
        fee += load != 0;
        a.worked += load;
    }
    if ((port & 32) && aligned && (a.load || a.fibre)) {
        bool strand = a.fibre && (!a.load || organic);
        bool attach = c.terrain && (g.grip & 4) && (!strand || c.weaving);
        Half &bound = strand ? weave : wall;
        auto &where = attach ? bound : strand ? facing.litter : facing.grains;
        bool joining = attach && !bound && (strand ? wall : weave);
        Word moved = pour(strand ? a.fibre : a.load, where, 1 + hook / 3);
        if(moved&&strand&&attach){practice::disturb(w.practice,c.side,pos,across);tag_strand(w,c.side,pos,across,weave-moved,weave);}
        fee += moved != 0;
        a.worked += moved;
        if (attach) {
            a.laid += !strand * moved;
            count(strand ? total.fibre_laid : total.laid, moved);
            count(total.joined, joining && moved);
        } else if (!strand)
            count(total.displaced, moved);
    }
    if (port & 64) {
        if (g.grip & 8) {
            target.enzyme = static_cast<Byte>(1 + (g.stomach >> 24) % 255);
            ++fee;
        }
        if (c.predation && aligned && other.energy > 1 && hook > nbits(other.body.limbs) / 2 && (g.grip & 16)) {
            Word moved = (g.grip & 32) ? pour(a.energy, other.energy, cut(a.energy - 1, hook / 2))
                                       : pour(other.energy, a.energy, cut(other.energy - 1, hook / 2));
            count(total.shared, moved);
        }
    }
    if (port & 128) {
        a.signal = 1 + ((g.voice >> (3 * action)) & 7);
        ++fee;
        count(total.calls, 1);
    }
    if(w.outlay.minds && (port&8))effort_meal(w,c,id,fruit_skin(w,pos),a.meals-before_meal);
    if (w.matter.bed)
        work_material(w, c, pos, across, edge_direction, id, action, tick, key, a.meals - before_meal);
    if (w.worksites.residents)
        worksite::practice(w, c, pos, across, edge_direction, id, action, tick, mix(key + 1871));
    Word rotation=port&3;
    if(w.hollows.minds) {
        if (a.meals>before_meal && ((w.hollows.minds[id].genes.compass>>9)&1))
            remember_here(w,c,id,pos,4*hollow::harvest+(site.spore&3),true,tick,key);
        hollow_work(w,c,id,pos,tick,key);
        if(w.outlay.minds)press_earth(w,c,id,pos,(edge_direction==1 || edge_direction==2)?pos:across,
                                     (edge_direction&1)?0u:1u,action,tick,key);
        Word surface=w.outlay.minds?stamped_skin(w,pos,(edge_direction&1)?0u:1u,w.outlay.beds[(edge_direction==1 || edge_direction==2)?pos:across]):0u;
        rotation=hollow_steer(w,c,id,pos,rotation,tick,key,surface);
    }
    a.heading = static_cast<Byte>((a.heading + rotation) & 3);
    payment(a, site, fee, total);
    if (a.energy && c.morph && (g.growth & (1u << 21)) && (tick & 63) == (a.born & 63)) {
        Germ growing = g;
        Word sensory = (site.water >= 32) + 2 * Word(a.load != 0) + 4 * Word(a.energy >= 64);
        if ((g.growth >> (24 + sensory)) & 1)
            growing.growth ^= (g.grip >> 20) & 15;
        auto new_shape = grow(growing);
        count(total.remodelled, new_shape.shape != a.body.shape);
        a.body = new_shape;
    }
}
AX_FN Word somatic_choice(MotorCounts scores,Word key) {
    Word start=key&7,action=start,best=0;
    for(Word j=0;j<8;++j){
        Word motor=(j+start)&7,drive=((motor<4?scores.low:scores.high)>>(8*(motor&3)))&255;
        if(!j||drive>best){action=motor;best=drive;}
    }
    return action;
}
AX_FN void inspect_bed(World w, Configuration c, Word pos) {
    if (w.matter.bed && !w.matter.observing)
        w.matter.attachment[pos] = material::conducts(w.matter.conditions) ? attachments(w.matter.bed, pos, c.side) : 0;
}
template <int Tiles, bool Packed = false>
AX_FN void think_group(World w, Configuration c, Word tick, Word pos, Word slot, Word tile, Word base, Word mask,
                       Word expressed = 0) {
    auto &a = w.animals[slot];
    const auto &g = w.dna[slot];
    Word active = 0, ahead = 0, input = 0, omission=0, auditory=0;
    if (!tile) {
        input = view(pos, w, c, tick, active, ahead);
        if(w.braids.minds && (w.braids.mode&braid::projecting)){
            const auto &organ=w.braids.minds[slot];
            Word projection=braid::afferent(organ);input^=projection;
            Word sample=mix(slot*20323u+tick);
            if(projection && !(sample&255) && !w.matter.observing){
                auditory=(sample>>8)&1;
                Word plane=auditory?0xffff0000u:65535u;
                omission=projection & rotate(plane,(organ.genes.arbor>>24)&31);
            }
        }
        if(w.hollows.minds) {
            const auto &m=w.hollows.minds[slot];
            if((m.efferent&16) && (m.genes.compass&(1u<<12)))
                input ^= rotate((1u<<(m.efferent&3)) | (1u<<(4+((m.efferent>>16)&15))), (m.genes.compass>>14)&31);
        }
        if (w.siphons.residents) {
            auto &p = w.siphons.residents[slot];
            Word heard = (!w.matter.observing) ? audience_pulse(w, c, pos, slot, tick, c.plastic) : 0;
            input ^= rotate(((p.volley & 4095) ^ (heard << 1)) * 0x01010101u, p.genes.lateral >> 26) & p.genes.volition;
            if (w.clay.minds) {
                auto &m = w.clay.minds[slot];
                Word signal = (1u << clay::active(m)) | (m.charge << 4);
                input ^= rotate(signal, m.genes.branches >> 24) & m.genes.membrane;
            }
        }
        if (w.worksites.residents) {
            if (!w.matter.observing && c.plastic)
                worksite::read(w, c, pos, slot, tick);
            input ^= worksite::afferent(w, c, pos, slot);
        }
        if (w.tide.cells && !w.matter.observing && c.plastic)
            see_material(w, c, pos, slot, tick);
        if (w.matter.bed) {
            Word target = neighbor(pos, a.heading, c.side);
            auto &man = w.matter.residents[slot];
            Word local =
                material::sense(w.matter.bed[pos], w.matter.bed[target], man.hand, a.heading, w.land[target].spore);
            input ^= rotate(local * 0x01010101u, man.genes.receptors >> 16);
        }
        if (a.heard && (c.wire & 3) && c.plastic) {
            auto &e = w.ears[slot];
            Word k = a.heard - 1;
            auto v = grounding::plastic({e.low[k], e.high[k]}, active, g.hearing.low);
            e.low[k] = v.low;
            e.high[k] = v.high;
            ++a.witnesses;
            ++e.visits[k];
        }
    }
    input = shuffle(input, base, mask);
    Word extent = Packed ? expressed : expressed_cells(w, slot, g.brain, Tiles);
    Word capacity = Packed ? c.tiles : Tiles;
    Word firing = extent;
    if (w.cortex.resident && (w.cortex.mute_new == 1 || (a.cohort && w.cortex.mute_new == 1u + a.cohort)))
        firing = conducting_bands(w.cortex.resident[slot], true);
    Tissue t = (!w.cortex.resident || tile < firing) ? read_band(w, capacity, slot, tile) : Tissue{};
    Genome brain = g.brain;
    Word kind = ((g.allocation >> (2 * (tile & 15))) & 3) % g.module_count;
    auto module = g.module[kind];
    brain.edges[0] ^= module.routing;
    brain.edges[1] ^= rotate(module.routing, 11);
    auto respond = [&](Tissue &part, Word sense) {
        if (Packed || w.cortex.resident)
            neural<Tiles, false, true>(brain, part, sense, tile, base, mask, c.plastic != 0, extent, firing);
        else
            neural<Tiles, false>(brain, part, sense, tile, base, mask, c.plastic != 0);
        Word remote=afferent<Tiles,true>(part.spike,module.routing,tile,extent,base,mask);
        if((!w.cortex.resident||tile<firing) && (module.gate&(1u<<21)))
            instruction(module.gate,{remote,part.memory,sense,mix(brain.development+tile)},part,c.plastic!=0);
        return motor_counts<Tiles, true, Packed>(brain, part, tile, base, mask, extent);
    };
    Tissue prior=t;
    auto scores=respond(t,input);
    if (!w.cortex.resident || tile < firing)
        write_band(w, capacity, slot, tile, t);
    if (!tile && w.neural_pages.index) {
        Word count = w.cortex.resident ? firing : c.tiles;
        w.neural_pages.overrides[slot] |= ~0u >> (32 - count);
    }
    Word key=mix(c.seed+tick*1499+slot*0x9e3779b9u);
    Word action=tile?0:somatic_choice(scores,key);
    if(w.braids.minds && (w.braids.mode&braid::projecting) && (omission=shuffle(omission,base,mask))){
        auto quiet=respond(prior,input^omission);
        Word changed = neural_sum<Tiles, Packed>(Word(tile < firing && prior.spike != t.spike), tile, base, mask);
        if(!tile){
            auto &b=w.braids.book[c.side>=128?slot/512:0];
            count(auditory?b.ear_events:b.soma_events,1);
            count(auditory?b.ear_changed:b.soma_changed,changed!=0);
            count(auditory?b.ear_moves:b.soma_moves,action!=somatic_choice(quiet,key));
        }
    }
    if (!tile) {
        if (w.pods.animal) {
            Word place = pod_reach(pos, a, g, w, c, tick);
            auto &organ = w.pods.animal[slot];
            Word raw = pods::light(w.land[place].spore, w.pods.skin[place].winding,
                                   optical(w.land[place], w.animals[w.slots[place]].energy));
            auto p = pods::pulse(organ.brain, organ.genes, organ.frame, organ.frame_genes, raw, action, key,
                                 pods::sensory_channels(w.pods.physics, a.cohort) & 4);
            if (!w.pods.observing)
                organ.emission = p.native | (p.favourite << 2) | (p.predictions << 4) | (Word(p.distinct) << 8) |
                                 (Word(p.anchored) << 9);
            action |= p.physical << 3 | p.cue << 5;
        }
        w.motor[slot] = static_cast<Byte>(action);
    }
}
template <int Tiles> AX_FN void think(World w, Configuration c, Word tick) {
    Word global = tid(), pos = global / Tiles;
    if (pos >= c.side * c.side)
        return;
    Word tile = global & (Tiles - 1), base = (global & 31) - tile;
    Word mask = Tiles == 32 ? ~0u : (~0u >> (32 - Tiles)) << base;
    Word slot = w.slots[pos];
    if (!tile)
        inspect_bed(w, c, pos);
    if (w.animals[slot].energy)
        think_group<Tiles>(w, c, tick, pos, slot, tile, base, mask);
}
#include "flood_neural_dispatch.cuh"
KERNEL void flood_gate_trial(World w, Word capacity, Word sites, Word tick, Word seed, Word phase, Word sham,
                             GateSubject *records) {
    Word slot = tid();
    if (slot >= sites)
        return;
    GateSubject record{};
    auto body = w.animals[slot];
    if (body.energy && body.born < tick) {
        // These natal bands have never been retired. Their original gate
        // bits are recoverable from the immutable developmental seed.
        Word kept = w.cortex.resident[slot].least;
        Word development = w.dna[slot].brain.development;
        Word key = mix(seed ^ slot), arm = (key % 3 + phase) % 3;
        for (Word band = 0; band < kept; ++band) {
            auto original = read_band(w, capacity, slot, band);
            auto birth = developmental_band(development, band);
            Word delta_a = original.gate_a ^ birth.gate_a, delta_b = original.gate_b ^ birth.gate_b;
            record.acquired += nbits(delta_a) + nbits(delta_b);
            if (sham || !arm || !(delta_a | delta_b))
                continue;
            auto value = original;
            Word shift = 1 + mix(key ^ (band * 0x9e3779b9u)) % 31;
            value.gate_a = birth.gate_a ^ (arm == 2 ? rotate(delta_a, shift) : 0u);
            value.gate_b = birth.gate_b ^ (arm == 2 ? rotate(delta_b, shift) : 0u);
            record.changed += nbits(value.gate_a ^ original.gate_a) + nbits(value.gate_b ^ original.gate_b);
            assign_band(w, capacity, slot, band, value);
        }
        if (record.acquired) {
            record.arm = 1 + arm;
            record.born = body.born;
            record.opening_energy = body.energy;
        }
    }
    records[slot] = record;
}
} // namespace
KERNEL void flood_nerve_inventory(World w, Word length, Word *live) {
    Word i = tid(), key = i / 32, lane = i & 31;
    if (key >= length)
        return;
    Word entry = page_entry(w.neural_pages.index + key), active = 0;
    if (entry) {
        Word first = (key % w.neural_pages.groups) * nerve_page_slots;
        for (Word j = lane; j < nerve_page_slots; j += 32)
            if (first + j < w.neural_pages.sites &&
                (w.neural_pages.overrides[first + j] & (1u << (basal_bands + key / w.neural_pages.groups))))
                active |= nonzero_nerve(*page_address(w, first + j, key, entry));
    }
    Word occupied = warp_ballot(active);
    if (!occupied && entry == spill_page) {
        for (Word j = lane; j < nerve_page_slots; j += 32)
            w.neural_pages.spill[Count(key) * nerve_page_slots + j] = {};
    }
    if (!lane)
        live[key] = occupied ? entry : 0;
}
KERNEL void flood_nerve_repack(World w, const Word *keys, const Word *destination, Word total, Tissue *regular) {
    Word i = tid();
    if (i >= total)
        return;
    Word key = keys[i / nerve_page_slots], offset = i % nerve_page_slots;
    Word slot = (key % w.neural_pages.groups) * nerve_page_slots + offset;
    if (slot >= w.neural_pages.sites)
        return;
    Word old = w.neural_pages.index[key], next = destination[key];
    if (old == spill_page && next == spill_page)
        return;
    Word band = basal_bands + key / w.neural_pages.groups;
    Tissue value = (w.neural_pages.overrides[slot] & (1u << band)) ? *page_address(w, slot, key, old) : Tissue{};
    if (next == spill_page)
        w.neural_pages.spill[Count(key) * nerve_page_slots + offset] = value;
    else
        regular[Count(next - 1) * nerve_page_slots + offset] = value;
    if (old == spill_page)
        w.neural_pages.spill[Count(key) * nerve_page_slots + offset] = {};
}
KERNEL void flood_nerve_spills(NervePages pages, const Word *keys, Word count, Tissue *result) {
    Word i = tid();
    if (i < count)
        result[i] = pages.spill[Count(keys[i / nerve_page_slots]) * nerve_page_slots + i % nerve_page_slots];
}
KERNEL void flood_nerve_import(World w, Word capacity, Count offset, Word count, const Tissue *input) {
    Word i = tid();
    if (i < count) {
        Count address = offset + i;
        import_band(w, capacity, address / capacity, address % capacity, input[i]);
    }
}
KERNEL void flood_nerve_export(World w, Word capacity, Count offset, Word count, Tissue *output) {
    Word i = tid();
    if (i < count) {
        Count address = offset + i;
        output[i] = read_band(w, capacity, address / capacity, address % capacity);
    }
}
KERNEL void flood_think_packed(World w, Configuration c, const Word *clock, Word offset) {
    packed_think(w, c, *clock + offset);
}
KERNEL void flood_seed(World w, Configuration c, const Germ *founder_dna, Word graft, Word initial_tick) {
    Word i = tid(), n = c.side * c.side;
    if (i >= n)
        return;
    Word key = mix(c.seed + i * 0x9e3779b9u);
    Germ g = egg(key);
    if (graft & 4)
        g = *founder_dna;
    if (graft & 1)
        g.brain = founder_dna->brain;
    if (graft)
        g.hearing = founder_dna->hearing;
    if (graft & 2)
        for (Word j = 0; j < 4; ++j)
            g.pigments[j] = founder_dna->pigments[j];
    Silt s;
    s.water = static_cast<Half>((mix(key + 21) & 63) + 8);
    s.grains = static_cast<Half>(6 + (mix(key + 22) & 31));
    s.root = c.flora ? mix(key + 1289) : 0;
    s.spore = static_cast<Byte>(mix(key + 75));
    s.leaf = static_cast<Half>(16 + (key & 63));
    s.fruit = static_cast<Half>(mix(key + 23) & 15);
    s.litter = static_cast<Half>(mix(key + 71) & 7);
    w.land[i] = s;
    w.slots[i] = i;
    Word x = i % c.side, y = i / c.side;
    bool nursery = !c.frontier || (x >= c.side / 4 && x < 3 * c.side / 4 && y >= c.side / 4 && y < 3 * c.side / 4);
    newborn(w, i, g, i, 0, initial_tick, nursery && (mix(key + 91) % 100) < c.density ? 48 + ((key >> 8) & 31) : 0, c);
}
// Each experimental founder is a newly developed embryo. The neutral census
// marker never enters receptor, motor, metabolic or reproduction calculations.
KERNEL void flood_colonists(World w, Configuration c, const Settler *a, Word na, const Settler *b, Word nb, Word swap,
                            Word cross, Word twins, Word paired_material) {
    Word p = tid();
    if (p >= c.side * c.side / 2)
        return;
    Word key = mix(c.seed ^ mix(p * 713 + 0x6fd32a7bu));
    Word x = 2 * (p % (c.side / 2)), y = p / (c.side / 2);
    bool nursery = !c.frontier || (x >= c.side / 4 && x < 3 * c.side / 4 && y >= c.side / 4 && y < 3 * c.side / 4);
    Word energy = nursery && mix(key + 91) % 100 < c.density ? 48 + (mix(key + 51) & 31) : 0;
    for (Word k = 0; k < 2; ++k) {
        Word id = cell_at(x + k, y, c.side);
        Word who = nb ? ((key >> 23) & 1) ^ k ^ swap : 0;
        auto first = a[mix(key + 317) % na];
        Germ chosen = first.body;
        auto taste = first.pod;
        auto developmental = first.growth;
        auto hands = first.hands;
        auto investigator = first;
        if (nb) {
            auto second = b[mix(key + (twins ? 317 : 1217)) % nb];
            chosen = who ? second.body : first.body;
            taste = who ? second.pod : first.pod;
            developmental = who ? second.growth : first.growth;
            hands = who ? second.hands : first.hands;
            investigator = who ? second : first;
            if (cross) {
                auto &other = who ? first.body : second.body;
                taste = who ? first.pod : second.pod;
                developmental = who ? first.growth : second.growth;
                hands = who ? first.hands : second.hands;
                investigator = who ? first : second;
                auto &target = chosen.brain;
                auto peripheral = target;
                target = other.brain;
                target.reproduction = peripheral.reproduction;
                target.body = peripheral.body;
                target.effectors[0] = peripheral.effectors[0];
                target.effectors[1] = peripheral.effectors[1];
                chosen.hearing = other.hearing;
                chosen.allocation = other.allocation;
                chosen.module_count = other.module_count;
                for (Word j = 0; j < 4; ++j)
                    chosen.module[j] = other.module[j];
            }
            if (paired_material) {
                chosen = first.body;
                taste = first.pod;
                developmental = first.growth;
                hands = first.hands;
                investigator = first;
                investigator.saline = who ? second.saline : first.saline;
            }
        }
        newborn(w, id, chosen, id, 0, 0, energy, c);
        w.animals[id].cohort = static_cast<Half>(who + 1);
        if (w.clay.minds)
            w.clay.minds[id] = {investigator.ceramic};
        if (w.siphons.residents)
            w.siphons.residents[id] = {investigator.phonation};
        if (w.brine.animals)
            w.brine.animals[id] = {investigator.saline, {}};
        if (w.worksites.residents && investigator.has_inquiry)
            w.worksites.residents[id] = {investigator.inquiry_receptors, investigator.inquiry_impulses};
        if (w.matter.bed) {
            w.matter.residents[id] = {};
            w.matter.residents[id].genes = hands;
        }
        if (w.cortex.resident)
            w.cortex.resident[id].genes = developmental;
        if (w.pods.animal) {
            w.pods.animal[id] = {};
            w.pods.animal[id].genes = taste.law;
            w.pods.animal[id].frame_genes = taste.frame;
        }
    }
}
KERNEL void flood_water(World w, Configuration c, const Word *clock, Word offset) {
    Word tick = *clock + offset, k = tid();
    if (k < c.side * c.side / 2) {
        if (c.side >= 128) {
            w.account += k / 128;
            if (w.worksites.ledger)
                w.worksites.ledger += k / 128;
            if (w.seams.account)
                w.seams.account += k / 128;
            if (w.siphons.ledger)
                w.siphons.ledger += k / 128;
        }
        if(w.wicks.ledger&&c.side>=128)w.wicks.ledger+=k/128;
        if(w.wicks.reeds.book&&c.side>=128){w.wicks.reeds.book+=k/128;w.wicks.reeds.social+=k/128;}
        currents(w, c, tick, k);
        if (w.matter.bed) {
            if (c.side >= 128)
                w.matter.ledger += k / 128;
            material_physics(w, c, pair(k, c.side, tick & 3), tick);
        }
    }
}
#define THOUGHT(n)                                                                                                    \
    KERNEL void flood_think_##n(World w, Configuration c, const Word *clock, Word offset) {                           \
        think<n>(w, c, *clock + offset);                                                                              \
    }
THOUGHT(2)
THOUGHT(4) THOUGHT(8) THOUGHT(16) THOUGHT(32) KERNEL
    void flood_meet(World w, Configuration c, const Word *clock, Word offset, rattle::Mind *rattlers,drift::Pedigree *pedigree) {
    Word tick = *clock + offset, i = tid();
    if (i >= c.side * c.side / 2)
        return;
    if (c.side >= 128) {
        w.account += i / 128;
        if (w.pods.ledger)
            w.pods.ledger += i / 128;
        if (w.cortex.ledger)
            w.cortex.ledger += i / 128;
    }
    // Rotate contact matchings independently of the water update.
    auto p = pair(i, c.side, (tick + 2) & 3);
    if(w.wicks.ledger&&c.side>=128)w.wicks.ledger+=i/128;
    if(w.wicks.reeds.book&&c.side>=128){w.wicks.reeds.book+=i/128;w.wicks.reeds.social+=i/128;}
    auto &left = w.land[p.a], &right = w.land[p.b];
    auto &wall = p.axis ? left.south : left.east;
    auto &weave = p.axis ? left.fibre_south : left.fibre_east;
    Word ids[]{w.slots[p.a], w.slots[p.b]}, key = mix(c.seed ^ mix(p.a + tick * 0x1c734785u)), dir = p.axis ? 2 : 1;
    if (w.matter.bed && c.side >= 128)
        w.matter.ledger += i / 128;
    if (w.worksites.ledger && c.side >= 128)
        w.worksites.ledger += i / 128;
    if (w.seams.account && c.side >= 128)
        w.seams.account += i / 128;
    if (w.clay.ledger && c.side >= 128)
        w.clay.ledger += i / 128;
    if(w.outlay.minds) {
        if(c.side>=128)w.outlay.book+=i/128;
        wear_edge(w,p.a,p.axis,tick,key);
        for(Word j=0;j<2;++j) finish_outing(w,c,ids[j],w.animals[ids[j]].energy);
    }
    if (w.hollows.minds) {
        if(c.side>=128) w.hollows.ledger+=i/128;
        hollow_weather(w,c,p.a,tick,key);hollow_weather(w,c,p.b,tick,mix(key+191));
    }
    auto &u = w.animals[ids[0]], &v = w.animals[ids[1]];
    bool alive[]{u.energy != 0, v.energy != 0};
    if(pedigree && c.side>=128)pedigree+=i/128;
    bool aligned[]{u.heading == dir, v.heading == (dir ^ 2u)};
    Word order = key & 1;
    for (Word q = 0; q < 2; ++q) {
        Word k = q ^ order;
        if (!alive[k])
            continue;
        auto &a = k ? v : u, &other = k ? u : v;
        auto &site = k ? right : left, &facing = k ? left : right;
        if (w.pods.animal)
            settle_crop(w.pods.animal[ids[k]], a, site, tick,
                        c.plastic && (!a.cohort || a.cohort != w.pods.physics.cut_cohort) ? w.pods.physics.credit : 0,
                        pods::sensory_channels(w.pods.physics, a.cohort), *w.account, *w.pods.ledger);
        act(a, other, w.dna[ids[k]], site, facing, wall, weave, aligned[k], w.motor[ids[k]], mix(key + q * 8123), c,
            tick, w, ids[k], k ? p.b : p.a, k ? p.a : p.b, k ? (dir ^ 2) : dir);
        if (!a.energy && w.pods.animal)
            empty_crop(w.pods.animal[ids[k]], site, *w.account, *w.pods.ledger);
    }
    if(w.wicks.reeds.minds)fold_reed(w,c,p.a,p.b,p.axis,key);
    if (w.clay.minds) {
        if (w.clay.material >> 2) {
            if ((w.clay.material >> 2) == 2)
                clay_lend(w, ids[0], ids[1], key);
            clay_rake(w, c, p.a, p.b, ids[0], ids[1], tick, mix(key + 7531));
            clay_rake(w, c, p.b, p.a, ids[1], ids[0], tick, mix(key + 9757));
        } else {
            clay_touch(w, c, p.a, ids[0], tick, mix(key + 7531));
            clay_touch(w, c, p.b, ids[1], tick, mix(key + 9757));
        }
    }
    if (w.clay.use & press::bedded) {
        temper_grain(w, c, p.a, tick, key);
        temper_grain(w, c, p.b, tick, mix(key + 997));
    }
    if (w.clay.use & press::moving) {
        for (Word q = 0; q < 2; ++q) {
            Word k = q ^ order;
            press_contact(w, c, k ? p.b : p.a, k ? p.a : p.b, ids[k], ids[k ^ 1], k ? (dir ^ 2) : dir, tick,
                          mix(key + 7727 + k * 581));
        }
    }
    if (w.siphons.residents) {
        if (c.side >= 128)
            w.siphons.ledger += i / 128;
        siphon_encounter(w, c, p.a, p.b, ids[0], ids[1], dir, tick, mix(key + 9631));
    }
    if (w.worksites.residents) {
        worksite::shed(w, p.a, p.b, ids[0]);
        worksite::shed(w, p.b, p.a, ids[1]);
    }
    if (w.matter.bed && c.plastic && u.energy && v.energy) {
        if (aligned[0])
            see_work(w, ids[0], ids[1], key);
        if (aligned[1])
            see_work(w, ids[1], ids[0], mix(key));
    }
    if(pedigree)for(Word j=0;j<2;++j)if(alive[j] && !w.animals[ids[j]].energy)
        count(pedigree->deaths[braid::drift_tag(w.braids.minds[ids[j]].lived)],1);
    if (w.cortex.resident)
        for (Word j = 0; j < 2; ++j) {
            auto &body = w.animals[ids[j]];
            if (alive[j] && !body.energy)
                count(w.cortex.ledger->deaths, 32 * w.cortex.resident[ids[j]].active);
            if (body.energy)
                grow_cortex(w, c, ids[j], body, j ? right : left, tick);
        }
    u.heard = (c.wire & 3) && u.energy && v.energy ? v.signal : 0;
    v.heard = (c.wire & 3) && u.energy && v.energy ? u.signal : 0;
    if ((c.wire & 3) >= 2) {
        auto relay = [&](Byte &heard, Word salt) {
            if (heard)
                heard = static_cast<Byte>(
                    1 + ((c.wire & 3) == 2 ? mix(key ^ salt) & 7 : (heard - 1 + (c.seed % 7 + 1)) & 7));
        };
        relay(u.heard, 0x13a891d5u);
        relay(v.heard, 0x5bd13ac9u);
    }
    count(w.account->heard, Word(u.heard != 0) + Word(v.heard != 0));
    if (bool(u.energy) == bool(v.energy))
        return;
    Word k = u.energy ? 0 : 1;
    auto &a = k ? v : u;
    auto &site = k ? right : left, &next = k ? left : right;
    auto &g = w.dna[ids[k]];
    Word mass = nbits(a.body.shape), ability = 1 + nbits(a.body.limbs), bearing = k ? dir ^ 2 : dir;
    Word price = 50 + ((g.brain.reproduction >> 16) & 31) + mass * 2;
    if (a.energy >= price && (tick - a.born) >= 3 + ((g.brain.reproduction >> 24) & 7)) {
        Mutation edit;
        auto offspring_dna = offspring(g, mix(key + 1247), c, edit);
        Word energy = a.energy * (3 + ((g.brain.reproduction >> 4) & 3)) / 12;
        a.energy -= static_cast<Half>(energy);
        Word sediment = w.animals[ids[1 - k]].load, organic = w.animals[ids[1 - k]].fibre;
        if(w.outlay.minds){
            finish_outing(w,c,ids[1-k],0);
            auto &parent=w.outlay.minds[ids[k]];
            if(effort::remaining(parent)){effort::brood(parent,energy);count(w.outlay.book->inherited,energy);}
        }
        newborn(w, ids[1 - k], offspring_dna, a.founder, a.generation + 1, tick + 1, energy, c);
        w.animals[ids[1 - k]].load = static_cast<Half>(sediment);
        w.animals[ids[1 - k]].fibre = static_cast<Half>(organic);
        auto &baby = w.animals[ids[1 - k]];
        baby.cohort = a.cohort;
        if(w.braids.minds){
            if(w.braids.contacts)w.braids.contacts[ids[1-k]]={};
            if(w.braids.episodes)w.braids.episodes[ids[1-k]]={};
            auto parent=w.braids.minds[ids[k]].genes;
            Word inheritance=w.braids.minds[ids[k]].lived&braid::lineage_bits;
            w.braids.minds[ids[1-k]]={braid::inherit(parent,mix(key+64921),c.architecture?4+((g.brain.reproduction>>8)&31):0)};
            w.braids.minds[ids[1-k]].lived=inheritance;
            if(pedigree)count(pedigree->births[braid::drift_tag(inheritance)],1);
        }
        if (rattlers) {
            Word rate = c.architecture ? 4 + ((g.brain.reproduction >> 8) & 31) : 0;
            rattlers[ids[1-k]] = {rattle::inherit(rattlers[ids[k]].genes, mix(key + 57881), rate)};
        }
        if(w.wicks.reeds.minds){auto &m=w.wicks.reeds.minds[ids[1-k]];m={runnel::inherit(w.wicks.reeds.minds[ids[k]].genes,mix(key+45763),c.architecture?4+((g.brain.reproduction>>8)&31):0)};}
        if(w.practice.minds)w.practice.minds[ids[1-k]].genes=practice::child(w.practice.minds[ids[k]].genes,mix(key+78211),c.architecture?4+((g.brain.reproduction>>8)&31):0);
        if(w.linkage.minds)w.linkage.minds[ids[1-k]].genes=linkage::child(w.linkage.minds[ids[k]].genes,mix(key+91331),c.architecture?4+((g.brain.reproduction>>8)&31):0);
        if(w.outlay.minds)w.outlay.minds[ids[1-k]].genes=effort::child(w.outlay.minds[ids[k]].genes,mix(key+71321),c.architecture?4+((g.brain.reproduction>>8)&31):0);
        bool changed_crop = false;
        if (w.hollows.minds) {
            w.hollows.minds[ids[1-k]].efferent=w.hollows.minds[ids[k]].efferent&hollow::lineage_tag;
            auto parent=w.hollows.minds[ids[k]].genes;
            w.hollows.minds[ids[1-k]].genes=hollow::child(parent,mix(key+23741),c.architecture?4+((g.brain.reproduction>>8)&31):0);
        }
        if (w.matter.bed) {
            auto &hand = w.matter.residents[ids[1 - k]];
            auto cargo = hand.hand;
            auto law = w.matter.residents[ids[k]].genes;
            hand = {};
            hand.hand = cargo;
            hand.genes =
                c.architecture ? material::inherit(law, mix(key + 57133), 4 + ((g.brain.reproduction >> 8) & 31)) : law;
            changed_crop = hand.genes.muscle != law.muscle || hand.genes.innate != law.innate ||
                           hand.genes.receptors != law.receptors || hand.genes.practice != law.practice ||
                           hand.genes.observation != law.observation || hand.genes.release != law.release;
            edit.edits += changed_crop;
        }
        if (w.siphons.residents) {
            auto germ = w.siphons.residents[ids[k]].genes;
            Word chance = c.architecture ? 4 + ((g.brain.reproduction >> 8) & 31) : 0;
            if (w.clay.minds) {
                auto &parent = w.clay.minds[ids[k]], &child = w.clay.minds[ids[1 - k]];
                child = {clay::inherit(parent.genes, mix(key + 14851), chance)};
                child.banks = parent.banks & 192u;
            }
            w.siphons.residents[ids[1 - k]] = {siphon::inherit(germ, germ, mix(key + 11863), chance, false)};
        }
        if (w.brine.animals) {
            auto &parent = w.brine.animals[ids[k]].genes;
            Word salt = mix(key + 73939);
            bool edit = c.architecture && ((salt & 255u) < 4 + ((g.brain.reproduction >> 8) & 31));
            w.brine.animals[ids[1 - k]] = {brine::descend(parent, parent, salt, edit, false), {}};
        }
        if (w.worksites.residents) {
            auto prior = w.worksites.residents[ids[k]];
            auto child =
                worksite::inherit(prior, mix(key + 3137), c.architecture ? 4 + ((g.brain.reproduction >> 8) & 31) : 0);
            w.worksites.residents[ids[1 - k]] = child;
            Word edited = Word(child.receptors != prior.receptors) + Word(child.impulses != prior.impulses);
            edit.edits += edited;
            changed_crop |= edited != 0;
        }
        if (w.cortex.resident) {
            auto &parent = w.cortex.resident[ids[k]], &child = w.cortex.resident[ids[1 - k]];
            auto &book = *w.cortex.ledger;
            child.genes = c.architecture ? inherit_cortex(parent.genes, key, 4 + ((g.brain.reproduction >> 8) & 31))
                                         : parent.genes;
            changed_crop |=
                child.genes.response != parent.genes.response || child.genes.regulation != parent.genes.regulation;
            edit.edits += Word(child.genes.response != parent.genes.response) +
                          Word(child.genes.regulation != parent.genes.regulation);
            count(book.births, 32 * child.natal);
            count(book.larger_births, child.natal > parent.natal);
            count(book.smaller_births, child.natal < parent.natal);
        }
        if (w.pods.animal) {
            auto law = w.pods.animal[ids[k]].genes;
            auto transform = w.pods.animal[ids[k]].frame_genes;
            w.pods.animal[ids[1 - k]] = {};
            w.pods.animal[ids[1 - k]].frame_genes =
                c.architecture ? pods::recruit(pods::frame_child(transform, mix(key + 0x79431),
                                                                 4 + ((g.brain.reproduction >> 8) & 31)),
                                               law, key)
                               : transform;
            w.pods.animal[ids[1 - k]].genes =
                c.architecture ? pods::descend(law, mix(key + 0xe951u), 4 + ((g.brain.reproduction >> 8) & 31)) : law;
            const auto &offspring = w.pods.animal[ids[1 - k]];
            Word changes = Word(law.plastic != offspring.genes.plastic) + Word(law.timing != offspring.genes.timing) +
                           Word(law.readout != offspring.genes.readout) + Word(law.sense != offspring.genes.sense);
            changes += Word(transform.learning != offspring.frame_genes.learning) +
                       Word(transform.routes != offspring.frame_genes.routes) +
                       Word(transform.consolidation != offspring.frame_genes.consolidation);
            edit.edits += changes;
            changed_crop |= changes != 0;
        }
        auto ancestral = grow(g);
        Count origin = (Count(tick + 1) << 32) | ids[1 - k];
        bool body_change = baby.body.shape != ancestral.shape || baby.body.limbs != ancestral.limbs ||
                           baby.body.eyes != ancestral.eyes || baby.body.pores != ancestral.pores;
        const auto &brain = offspring_dna.brain;
        Word length = 1 + (g.brain.expression & 7);
        bool neural_change = changed_crop || (brain.expression & 7) != (g.brain.expression & 7) ||
                             brain.edges[0] != g.brain.edges[0] || brain.edges[1] != g.brain.edges[1] ||
                             brain.development != g.brain.development || brain.receptors != g.brain.receptors ||
                             brain.motors != g.brain.motors;
        for (Word t = 0; t < living_tiles(c.tiles, brain.development) && !neural_change; ++t) {
            auto old = g.module[((g.allocation >> (2 * (t & 15))) & 3) % g.module_count];
            auto next =
                offspring_dna.module[((offspring_dna.allocation >> (2 * (t & 15))) & 3) % offspring_dna.module_count];
            neural_change = old.routing != next.routing ||
                            ((old.gate ^ next.gate) & 0x3fffffu && ((old.gate | next.gate) & (1u << 21)));
        }
        count(w.account->module_edits, edit.modules);
        for (Word k = 0; k < length && !neural_change; ++k)
            neural_change = brain.op[k] != g.brain.op[k];
        baby.body_origin = body_change ? origin : a.body_origin;
        baby.brain_origin = neural_change ? origin : a.brain_origin;
        ++a.offspring;
        count(w.account->births, 1);
        count(w.account->edits, edit.edits);
        count(w.account->tape_edits, edit.tape);
        count(w.account->body_edits, edit.body);
        count(w.account->brain_changes, neural_change);
        return;
    }
    // A water head across the paired banks can carry a body without a step.
    // Grain walls, woven fibre and the animal's footing resist displacement.
    if((w.hollows.mode&hollow::drift) && site.water>=8) {
        Word head=site.water+site.grains/6,lower=next.water+next.grains/6;
        Word footing=2+wall+weave/2+mass/4+ability/2+(a.load+a.fibre)/4;
        if(head>lower+footing && ((key>>18)&15)<cut(12,head-lower-footing)) {
            if(w.practice.minds)w.practice.minds[ids[k]].focus=0;
            w.slots[p.a]=ids[1];w.slots[p.b]=ids[0];
            hollow_step(w,ids[k],k?p.b:p.a,0,false);
            ++a.motions;count(w.account->moves,1);return;
        }
    }
    bool called=w.hollows.minds && (w.hollows.minds[ids[k]].efferent&16);
    bool suppressed=w.outlay.minds && (w.hollows.minds[ids[k]].efferent&(1u<<13));
    if (a.heading != bearing || (!(effector(g.brain, w.motor[ids[k]] & 7) & 4) && !called) || suppressed)
        return;
    Word depth = next.water / 48, dry = 1 + (a.load + a.fibre) / 4 + mass / 7 + wall / 2,
         force = 2 + ability + ((g.grip >> 5) & 3);
    Word support = cut(depth, weave / (1 + mass / 4)), drag = dry + depth - support;
    Word fee=1,virgin=1;
    if(w.outlay.minds){
        Word present=k?p.b:p.a;
        virgin=crossing_fee(w,c,present,p.a,p.axis,tick,mass,wall,weave,0);
        fee=crossing_fee(w,c,present,p.a,p.axis,tick,mass,wall,weave,w.outlay.beds[p.a]);
    }
    if (drag > force || a.energy <= fee) {
        if(called) w.hollows.minds[ids[k]].efferent|=1u<<8;
        count(w.account->blocked, 1);
        return;
    }
    if(w.practice.minds && (w.practice.mode&practice::holding) && practice::stays(w.practice.minds[ids[k]],practice::tile_at(k?p.b:p.a,c.side))){
        auto *book=w.practice.book+(c.side>=128?i/256:0);count(book->stayed,1);return;
    }
    count(w.account->supported, dry + depth > force);
    if(w.practice.minds)w.practice.minds[ids[k]].focus=0;
    w.slots[p.a] = ids[1];
    w.slots[p.b] = ids[0];
    if (w.hollows.minds) hollow_step(w,ids[k],k?p.b:p.a,0);
    ++a.motions;
    count(w.account->moves, 1);
    if(w.outlay.minds){
        Word skin=stamped_skin(w,k?p.b:p.a,p.axis,w.outlay.beds[p.a]);
        if(c.plastic){effort::strain(w.outlay.minds[ids[k]],skin,fee,w.outlay.mode);
            if(w.outlay.mode&effort::appraisal)effort::recognise_traffic(w.outlay.minds[ids[k]],skin,w.outlay.mode);}
        footfall(w,p.a,p.axis,mass);
        if(c.plastic && a.body.eyes)effort::change(w.outlay.minds[ids[k]],skin,0,stamped_skin(w,k?p.b:p.a,p.axis,w.outlay.beds[p.a]),w.outlay.mode);
        count(w.outlay.book->walks,1);count(w.outlay.book->price,fee);
        count(w.outlay.book->fresh_price,virgin);count(w.outlay.book->saved,virgin-fee);
    }
    payment(a, next, fee, *w.account);
    if(pedigree && !a.energy)count(pedigree->deaths[braid::drift_tag(w.braids.minds[ids[k]].lived)],1);
    if (!a.energy && w.pods.animal)
        empty_crop(w.pods.animal[ids[k]], next, *w.account, *w.pods.ledger);
}
KERNEL void braid_seed(World w,Word n,Word side,Word salt) {
    Word i=tid();if(i>=n)return;
    w.braids.minds[i]={braid::founder(mix(salt+i*1571))};
    if(w.braids.contacts)w.braids.contacts[i]={};
    w.braids.floats[i]=braid::floater(mix(salt+i*3911)&3,i%side,i/side);
}
KERNEL void braid_gates(World w,Configuration c,const Word *clock,Word offset){
    Word tick=*clock+offset,region=8*tid()+(tick&7);
    if(region<c.side*c.side/8)braid::gateway(w,c,region,tick);
}
KERNEL void braid_remoor(braid::Half *floats,Word n,Word side,Word seed){
    Word i=tid();if(i<n)floats[i]=braid::floater(mix(seed+i*3911)&3,i%side,i/side);
}
KERNEL void braid_works(World w, Configuration c, const Word *clock, Word offset) {
    Word tick = *clock + offset, tile = 4 * tid() + (tick & 3);
    if (tile < c.side * c.side / 4)
        braid::mooring(w, c, tile, tick, [](Word, braid::Mind &m, Word motor, Word opening, Word outcome, Word) {
            return braid::motor_touch(m, motor, opening, outcome, false);
        });
}
AX_FN Word motor_band(World w, Configuration c, Word slot, bool voice = false) {
    const auto &g = w.dna[slot];
    Word width = expressed_cells(w, slot, g.brain, c.tiles);
    auto tag = w.animals[slot].cohort;
    if (w.cortex.resident && (w.cortex.mute_new == 1 || (tag && w.cortex.mute_new == 1u + tag)))
        width = conducting_bands(w.cortex.resident[slot], true);
    return voice && (w.braids.mode & regulation::auditory_port) ? regulation::auditory_band(g, width)
                                                                : regulation::innervated_band(g, width);
}
KERNEL void shore_carriers(World w, Configuration c, apprentice::Donor donor, Word start,
                           apprentice::Carriers *counts) {
    Word i = tid();
    if (i >= c.side * c.side)
        return;
    auto &a = w.animals[i];
    if (!a.energy)
        return;
    auto &totals = counts[i / 128];
    auto &g = w.dna[i];
    Word band = motor_band(w, c, i);
    auto module = g.module[regulation::module_index(g, band)];
    Word distance = nbits(module.routing ^ donor.program.first) + nbits(module.gate ^ donor.program.second);
    auto law = w.braids.minds[i].genes.plastic;
    if (donor.coupled) {
        Word active = (w.braids.minds[i].genes.arbor >> 12) & 1;
        distance += nbits(drift::lateral_hand(law & 15,
                                              drift::wiring(w.braids.minds[i].genes,
                                                            donor.coupled == 2 ? Word(drift::recruitment) : 0u),
                                              active) ^
                          drift::lateral_hand(donor.law, donor.wiring, donor.hand_active));
    }
    count(totals.alive, 1);
    count(totals.exact, distance == 0);
    count(totals.newborn, !distance && a.born > start && a.generation != 0);
    count(totals.both_ports, !distance && (law & 15) == donor.law && ((law >> 12) & 15) == donor.law);
    count(totals.neighbours, distance == 1);
}
KERNEL void braid_regulatory_sources(World w, Configuration c, const Word *ids, Word size,
                                     apprentice::Teachers *organs) {
    Word i = tid();
    if (i >= size)
        return;
    Word slot = ids[i], band = motor_band(w, c, slot);
    apprentice::Teachers tissue{};
    tissue.motor = regulation::source(w.dna[slot], band, read_band(w, c.tiles, slot, band));
    Word voice = motor_band(w, c, slot, true);
    if (voice != band)
        tissue.voice = regulation::source(w.dna[slot], voice, read_band(w, c.tiles, slot, voice));
    organs[i] = tissue;
}
KERNEL void braid_works_regulated(World w, Configuration c, const Word *clock, Word offset,
                                  regulation::Ledger *totals) {
    Word tick = *clock + offset, tile = 4 * tid() + (tick & 3);
    if (tile >= c.side * c.side / 4)
        return;
    auto &book = totals[c.side >= 128 ? tile / 128 : 0];
    braid::mooring(
        w, c, tile, tick, [&](Word slot, braid::Mind &mind, Word motor, Word opening, Word outcome, Word place) {
            return braid::receive_contact(
                mind, motor, braid::compatible(opening, outcome, mind.genes.arbor & 3), 0,
                [&](Word old, Word conflict, Word proposed) {
                    auto aim = braid::predict(braid::matrix(mind, motor), opening);
                    bool attempted = aim.known & (1u << place),
                         matched = attempted && braid::pigment(aim.colors, place) == braid::pigment(outcome, place);
                    bool connected = !(w.braids.mode & regulation::silent_feedback);
                    Word band = motor_band(w, c, slot);
                    auto tissue = read_band(w, c.tiles, slot, band);
                    auto organ = regulation::source(w.dna[slot], band, tissue);
                    Word mask = regulation::discharge(organ.program, organ.cells, old, conflict, proposed,
                                                      connected && attempted, connected && matched);
                    proposed=regulation::candidate(organ.program,organ.cells.spike,old,proposed,w.braids.mode&regulation::structural_port);
                    Word result = regulation::apply(old, proposed, mask),
                         flips = nbits(tissue.spike ^ organ.cells.spike);
                    if (flips) {
                        tissue.spike = organ.cells.spike;
                        assign_band(w, c.tiles, slot, band, tissue);
                    }
                    count(book.contacts, 1);
                    count(book.attempted, attempted);
                    count(book.matched, matched);
                    count(book.proposed, nbits(old ^ proposed));
                    count(book.admitted, nbits(old ^ result));
                    count(book.releases, nbits(mask));
                    count(book.soma_changes, flips);
                    return result;
                });
        });
}
#include "flood_shore.cuh"
#include "flood_drift.cuh"
#include "flood_inlet.cuh"
#include "flood_retina.cuh"
#include "flood_capillary.cuh"
#include "flood_detritus.cuh"
KERNEL void rattle_seed(rattle::Mind *minds, Word n, Word salt) {
    Word i = tid();
    if (i < n) minds[i] = {rattle::founder(mix(salt + i * 4153))};
}
KERNEL void rattle_banks(World w, Configuration c, rattle::Field field, const Word *clock, Word offset) {
    Word tick = *clock + offset, tile = 4 * tid() + (tick & 3);
    if (tile < c.side * c.side / 4) rattle::cove(w, c, field, tile, tick);
}
KERNEL void flood_audit(World w, Configuration c, BasinCensus *partial) {
    Word i = tid(), t = i & 127;
    BasinCensus p;
    if (i < c.side * c.side) {
        auto s = w.land[i];
        Word slot = w.slots[i];
        auto a = w.animals[slot];
        p.water = s.water;
        p.banked = s.east + s.south;
        p.carried = a.load;
        p.clay = s.grains + p.banked + p.carried;
        p.leaf = s.leaf;
        p.fruit = s.fruit;
        p.litter = s.litter;
        p.energy_in_animals = a.energy;
        p.fibre = s.fibre_east + s.fibre_south;
        p.fibre_cargo = a.fibre;
        p.mixed_faces = Word(s.east && s.fibre_east) + Word(s.south && s.fibre_south);
        p.digesting = w.pods.animal ? pods::content(w.pods.animal[slot].gut) : 0;
        if (w.matter.bed) {
            auto grain = w.matter.bed[i], cargo = w.matter.residents[slot].hand;
            p.carried_grains = material::quantity(cargo);
            p.grains = material::quantity(grain) + p.carried_grains;
            p.excitation = grain.charge + cargo.charge;
            for (Word j = 0; j < 3; ++j)
                p.terminals[j] = material::inventory(grain, j + 1) + material::inventory(cargo, j + 1);
        }
        p.macromolecules = w.seams.stock ? w.seams.stock[i] : 0;
        p.energy = (w.braids.floats?braid::mass(w.braids.floats[i]):0) + (w.hollows.beds?w.hollows.beds[i].carbon:0) + p.macromolecules + p.excitation + p.leaf + p.fruit + p.litter + a.energy + p.fibre + p.fibre_cargo +
                   p.digesting;
        p.walls = Word(s.east != 0) + Word(s.south != 0);
        p.wet = s.water >= 32;
        if (a.energy) {
            p.alive = 1;
            p.area = nbits(a.body.shape);
            p.neurons = 32 * expressed_cells(w, slot, w.dna[slot].brain, c.tiles);
            bool silenced = w.cortex.mute_new == 1 || (a.cohort && w.cortex.mute_new == 1u + a.cohort);
            p.conducting = w.cortex.resident ? 32 * conducting_bands(w.cortex.resident[slot], silenced) : p.neurons;
            p.heard = a.witnesses;
            p.generations = a.generation;
        }
    }
    __attribute__((shared)) static Count sums[128];
    auto *values = reinterpret_cast<Count *>(&p);
    for (Word k = 0; k < sizeof(BasinCensus) / sizeof(Count); ++k) {
        sums[t] = values[k];
        asm volatile("bar.sync 0;" ::: "memory");
        for (Word step = 64; step; step >>= 1) {
            if (t < step)
                sums[t] += sums[t + step];
            asm volatile("bar.sync 0;" ::: "memory");
        }
        if (!t)
            reinterpret_cast<Count *>(partial + (i / 128))[k] = sums[0];
        asm volatile("bar.sync 0;" ::: "memory");
    }
}
KERNEL void flood_clock(Word *now, Word delta) { *now += delta; }
KERNEL void flood_unbank(Silt *land, Word n, Word *refused) {
    Word i = tid();
    if (i >= n)
        return;
    auto &s = land[i];
    if (Word(s.grains) + s.east + s.south > 65535 || Word(s.litter) + s.fibre_east + s.fibre_south > 65535) {
        asm volatile("red.global.add.u32 [%0],1;" ::"l"(refused) : "memory");
        return;
    }
    s.grains += s.east + s.south;
    s.east = s.south = 0;
    s.litter += s.fibre_east + s.fibre_south;
    s.fibre_east = s.fibre_south = 0;
}
KERNEL void flood_legacy_difference(const Silt *intact, const Silt *flat, Word n, LegacyDifference *out) {
    Word i = tid();
    if (i >= n)
        return;
    auto a = intact[i], b = flat[i];
    auto &d = out[i];
    d.water = __builtin_abs(int(a.water) - int(b.water));
    d.food = __builtin_abs(int(a.leaf) - int(b.leaf)) + __builtin_abs(int(a.fruit) - int(b.fruit)) +
             __builtin_abs(int(a.litter) - int(b.litter));
    d.wetter = a.water > b.water;
    d.vegetation = a.leaf > b.leaf;
}
KERNEL void flood_extend(World w, const Silt *land, const Word *slots, Word old_side, Word side) {
    Word i = tid();
    if (i >= side * side)
        return;
    Word x = i % side, y = i / side;
    if (x < old_side && y < old_side) {
        Word old = x + y * old_side;
        w.land[i] = land[old];
        w.slots[i] = slots[old];
    } else {
        Word preceding = cut(y, old_side) * old_side + (y < old_side ? cut(x, old_side) : 0);
        w.slots[i] = old_side * old_side + i - preceding;
    }
}
KERNEL void flood_margin(World w, Word side, Word *reached) {
    Word t = tid();
    if (t >= 2 * side)
        return;
    Word i = t < side ? side - 1 + side * t : (side - 1) * side + t - side;
    auto a = w.animals[w.slots[i]];
    if (a.energy && a.offspring)
        asm volatile("red.global.or.b32 [%0],1;" ::"l"(reached) : "memory");
}

// Consecutive counters are contiguous across a warp. Reduction is needed
// only at a census, before saving, or on a terrain-only intervention.
KERNEL void flood_budget(const Counters *shards, Word n, Counters *result) {
    Word column = tid();
    if (column >= sizeof(Counters) / sizeof(Count))
        return;
    Count sum = 0;
    for (Word j = 0; j < n; ++j)
        sum += reinterpret_cast<const Count *>(shards + j)[column];
    reinterpret_cast<Count *>(result)[column] = sum;
}

KERNEL void flood_listen_copy(World source, World trial, Configuration c, Word intervention) {
    Word i = tid();
    if (i >= c.side * c.side)
        return;
    trial.animals[i] = source.animals[i];
    trial.motor[i] = 0;
    Ear ear = source.ears[i];
    if (intervention == 1)
        ear = {};
    if (intervention == 2) {
        const auto &before = source.ears[i];
        for (Word k = 0; k < call_types; ++k) {
            Word other = (k + 1) % call_types;
            ear.low[k] = before.low[other];
            ear.high[k] = before.high[other];
            ear.visits[k] = before.visits[other];
        }
    }
    trial.ears[i] = ear;
    for (Word j = 0; j < c.tiles; ++j)
        write_band(trial, c.tiles, i, j, read_band(source, c.tiles, i, j));
}
KERNEL void flood_input(World world, Configuration c, const Word *clock, Word *input) {
    Word i = tid();
    if (i >= c.side * c.side)
        return;
    Word present = 0, front = 0;
    input[world.slots[i]] = view(i, world, c, *clock, present, front);
}
KERNEL void flood_hearing_effect(World original, World first, World other, Configuration c, const Word *first_input,
                                 const Word *other_input, HearingEffect *results) {
    Word i = tid();
    if (i >= c.side * c.side)
        return;
    HearingEffect out;
    const auto &a = original.animals[i];
    if (a.energy && a.heard && original.ears[i].visits[a.heard - 1]) {
        out.experienced = 1;
        out.inputs = first_input[i] != other_input[i];
        out.motor = first.motor[i] != other.motor[i];
        for (Word t = 0; t < expressed_cells(original, i, original.dna[i].brain, c.tiles); ++t) {
            auto s = first.nerve[Count(i) * c.tiles + t], o = other.nerve[Count(i) * c.tiles + t];
            out.spikes += nbits(s.spike ^ o.spike);
            out.contacts += nbits(s.gate_a ^ o.gate_a) + nbits(s.gate_b ^ o.gate_b);
        }
    }
    results[i] = out;
}

KERNEL void flood_rootstock(Silt *land, Configuration c) {
    Word i = tid();
    if (i < c.side * c.side)
        land[i].root = mix(c.seed ^ mix(land[i].spore + 1291 * i));
}

KERNEL void flood_mark(World w, Configuration c, Word *tags) {
    Word t = tid(), n = c.side * c.side;
    if (t >= 8)
        return;
    Word start = (mix(c.seed + t * 173) % n), limit = cut(n, 128);
    tags[2 * t] = ~0u;
    tags[2 * t + 1] = 0;
    for (Word j = 0; j < limit; ++j) {
        Word site = (start + j) & (n - 1), slot = w.slots[site];
        auto &a = w.animals[slot];
        if (a.energy) {
            tags[2 * t] = slot;
            tags[2 * t + 1] = a.born;
            break;
        }
    }
}
KERNEL void flood_track(World w, Configuration c, const Word *tags, Word *positions) {
    Word i = tid();
    if (i >= c.side * c.side)
        return;
    Word slot = w.slots[i];
    auto &a = w.animals[slot];
    if (!a.energy)
        return;
    for (Word j = 0; j < 8; ++j)
        if (slot == tags[2 * j] && a.born == tags[2 * j + 1])
            positions[j] = i;
}

KERNEL void flood_pod_setup(World w, Configuration c, Word origin, pods::Inheritance donor) {
    Word i = tid();
    if (i >= c.side * c.side)
        return;
    Word seed = mix(c.seed ^ mix(i + 179117));
    w.pods.skin[i].winding = static_cast<Byte>(mix(seed + 5) & 3);
    w.pods.animal[i] = {};
    w.pods.animal[i].genes = origin == 3   ? donor.law
                             : origin == 2 ? pods::Genes{}
                             : origin == 1 ? pods::reference()
                                           : pods::founder(seed);
    w.pods.animal[i].frame_genes = origin == 3   ? donor.frame
                                   : origin == 1 ? pods::frame_reference()
                                   : origin == 2 ? pods::FrameGenes{}
                                                 : pods::frame_founder(seed);
}
KERNEL void flood_pod_erase(pods::Animal *animals, Word n) {
    Word i = tid();
    if (i < n) {
        animals[i].brain = {};
        animals[i].frame = {};
        animals[i].emission = 0;
    }
}
KERNEL void flood_pod_budget(const pods::Census *records, Word n, pods::Census *out) {
    Word col = tid();
    if (col >= sizeof(pods::Census) / sizeof(Count))
        return;
    Count total = 0;
    for (Word i = 0; i < n; ++i)
        total += reinterpret_cast<const Count *>(records + i)[col];
    reinterpret_cast<Count *>(out)[col] = total;
}
KERNEL void flood_pod_extend(pods::Surface *now, const pods::Surface *old, Word prior, Word side) {
    Word i = tid();
    if (i < prior * prior)
        now[i % prior + (i / prior) * side] = old[i];
}

#include "flood_pod_assay.cuh"

#include "flood_pod_rafts.cuh"

#include "flood_pod_transfer.cuh"

#include "flood_pod_deploy.cuh"

KERNEL void flood_cortex_adopt(World w, Configuration c, Word graft, GrowthGenes founder) {
    Word slot = tid();
    if (slot >= c.side * c.side)
        return;
    auto &g = w.dna[slot].brain;
    Word active = living_tiles(c.tiles, g.development);
    auto genes = graft ? founder : growth_founder(mix(c.seed + slot + 19561));
    w.cortex.resident[slot] = hatch_cortex(genes, active);
    for (Word j = active; j < c.tiles; ++j)
        import_band(w, c.tiles, slot, j, {});
}

KERNEL void flood_growth_effect(World source, World intact, World muted, Configuration c, HearingEffect *out) {
    Word i = tid();
    if (i >= c.side * c.side)
        return;
    HearingEffect effect;
    const auto &stage = source.cortex.resident[i];
    if (source.animals[i].energy && stage.active > stage.natal) {
        effect.experienced = 1;
        effect.motor = (intact.motor[i] & 7) != (muted.motor[i] & 7);
        for (Word t = 0; t < stage.active; ++t) {
            auto a = intact.nerve[Count(i) * c.tiles + t], b = muted.nerve[Count(i) * c.tiles + t];
            if (t >= stage.natal)
                effect.inputs += nbits(a.spike);
            else {
                effect.spikes += nbits(a.spike ^ b.spike);
                effect.contacts += nbits(a.gate_a ^ b.gate_a) + nbits(a.gate_b ^ b.gate_b);
            }
        }
    }
    out[i] = effect;
}

KERNEL void flood_material_seed(World w, Configuration c, Word inherited, material::Genes donor) {
    Word i = tid();
    if (i >= c.side * c.side)
        return;
    w.matter.bed[i] = material::deposit(i, c.seed, w.matter.conditions.density);
    w.matter.residents[i] = {};
    w.matter.residents[i].genes = inherited ? donor : material::founder(mix(c.seed + i * 713 + 311));
}
KERNEL void flood_material_extend(material::Grain *next, const material::Grain *old, Word side, Word prior) {
    Word i = tid();
    if (i < prior * prior)
        next[i % prior + (i / prior) * side] = old[i];
}

#include "flood_material_network.cuh"

#include "flood_mill.cuh"
#include "flood_workbench.cuh"
#include "flood_quarry.cuh"
#include "flood_material_deploy.cuh"
#include "flood_lineage.cuh"

#include "axion/artifacts.hpp"
#include "flood_artifacts.cuh"

#include "flood_scree.cuh"

KERNEL void flood_inquiry_seed(World w, Configuration c) {
    Word i = tid();
    if (i < c.side * c.side)
        w.worksites.residents[i] = worksite::founder(mix(c.seed + i * 1277));
}
KERNEL void flood_imprint_extend(material::Imprint *field, const material::Imprint *prior, Word side, Word old) {
    Word i = tid();
    if (i < old * old)
        field[i % old + (i / old) * side] = prior[i];
}

#include "flood_field_transfer.cuh"
#include "flood_brine.cuh"
#include "flood_brine_image.cuh"

KERNEL void flood_sediment_extend(Half *field, const Half *old, Word side, Word prior) {
    Word i = tid();
    if (i < prior * prior)
        field[(i % prior) + (i / prior) * side] = old[i];
}

#include "flood_field_voice.cuh"
#include "flood_brine_voice.cuh"

KERNEL void press_catchment(World w, Configuration c, Word seed, Word tick, press::Survey *out, Word *histogram) {
    Word pos = tid(), n = c.side * c.side;
    if (pos >= n)
        return;
    press::Survey r{};
    Word id = w.slots[pos];
    const auto &body = w.animals[id];
    if (!body.energy || !body.body.eyes || !body.body.limbs) {
        out[pos] = r;
        return;
    }
    const auto &m = w.clay.minds[id];
    Word k = mix(seed + id * 541);
    Word foreign_id = id;
    for (Word j = 0; j < 32; ++j) {
        Word guest = w.slots[mix(k + j * 97) & (n - 1)];
        if (guest != id && w.animals[guest].energy) {
            foreign_id = guest;
            break;
        }
    }
    const auto &guest = w.clay.minds[foreign_id];
    auto sensory = m, appetite = m;
    sensory.charge = (m.charge & 65535) | (guest.charge & 0xffff0000u);
    appetite.banks = (m.banks & 65535) | (guest.banks & 0xffff0000u);
    r.live = 1;
    r.seen = bool(m.charge >> 16);
    r.valued = bool(m.banks >> 16);
    for (Word dir = 0; dir < 4; ++dir) {
        Word other = neighbor(pos, dir, c.side);
        for (Word hand = 0; hand < 8; ++hand) {
            auto table =
                press_table(w, pos, other, id, w.slots[other], dir, hand, press_layer(c, pos, tick, w.clay.use));
            const auto &a = table.tools[0], &b = table.tools[1];
            if (!a.body->faces || !b.body->faces || !table.stock || !w.land[pos].water)
                continue;
            ++r.choices;
            Word yield[2]{press::flow(press::ring(*a.surface, *a.body, a.face, w.clay.use & press::bedded), table.slit,
                                      w.land[pos].water, table.stock),
                          press::flow(press::ring(*b.surface, *b.body, b.face, w.clay.use & press::bedded), table.slit,
                                      w.land[pos].water, table.stock)};
            if (yield[0] == yield[1] || a.cue == b.cue)
                continue;
            ++r.useful;
            Word x = pos % c.side, y = pos / c.side, patch = x / 16 + (y / 16) * ((c.side + 15) / 16),
                 fold = ((x >> 2) ^ (y >> 2)) & 1;
            bool ascending = a.cue < b.cue;
            Word label = ascending ? (a.cue * 4 + b.cue) : (b.cue * 4 + a.cue);
            Word address = (((patch * 2 + fold) * 64 + table.bath * 16 + label) * 2) +
                           Word(ascending ? yield[1] != 0 : yield[0] != 0);
            Word unused;
            asm volatile("atom.global.add.u32 %0, [%1], 1;" : "=r"(unused) : "l"(histogram + address) : "memory");
            Word key = mix(k + dir * 907 + hand * 131);
            Word rim = rim_vibration(table, w.land[pos], c.seed), use = w.clay.use | press::choosing;
            auto recorded = feel_table(table, m, use, key, body.energy, true, rim, true);
            auto shifted =
                feel_table(table, m, use | press::dislocated, key, body.energy, true, rim, true, recorded.schedule);
            auto yoked_eye = feel_table(table, sensory, use, key, body.energy, true, rim, true, recorded.schedule);
            auto yoked_taste = feel_table(table, appetite, use, key, body.energy, true, rim, true, recorded.schedule);
            Word move[6]{recorded.chosen,
                         key & 1,
                         yoked_eye.chosen,
                         yoked_taste.chosen,
                         press::choose(m, a.cue, b.cue, table.bath, key, true),
                         shifted.chosen};
            Word cost[6]{recorded.cost, 0, yoked_eye.cost, yoked_taste.cost, 0, shifted.cost};
            for (Word arm = 0; arm < 6; ++arm) {
                r.correct[arm] += bool(yield[move[arm]]);
                r.mass[arm] += yield[move[arm]];
                r.work[arm] += 1 + cost[arm];
            }
        }
    }
    out[pos] = r;
}
KERNEL void hollow_color(hollow::Field field,Word seed,Word n) {
    Word i=tid();
    if(i<n) field.minds[i].efferent=(field.minds[i].efferent&~hollow::lineage_tag)|(mix(seed+i*7019)&hollow::lineage_tag);
}
KERNEL void hollow_found(hollow::Field f,Word seed,Word n) {
    Word i=tid();if(i>=n) return;
    f.minds[i]={hollow::founder(mix(seed+i*8929))};f.beds[i]={};
    for(Word j=0;j<f.slots;++j) {
        f.marks[Count(j)*n+i]=0;
        if(!(i&15)) f.sites[Count(j)*((n+15)/16)+i/16]=0;
    }
}
KERNEL void hollow_widen(hollow::Field f,const hollow::Bed *soil,const Word *audits,Word side,Word old) {
    Word i=tid(),n=old*old;if(i>=n) return;
    f.beds[(i/old)*side+i%old]=soil[i];
    if(!(i&15)) for(Word j=0;j<f.slots;++j) {
        Word v=audits[Count(j)*((n+15)/16)+i/16],location=v&0x7fffffffu;
        if(location) {--location;location=1+(location/old)*side+location%old;}
        f.sites[Count(j)*((f.stride+15)/16)+i/16]=(v&0x80000000u)|location;
    }
}
KERNEL void hollow_catchment(World w,Configuration c,Word seed,Word turn,Word minimum,hollow::FieldTest *out) {
    Word pos=tid(),n=c.side*c.side;if(pos>=n) return;
    hollow::FieldTest r{};Word id=w.slots[pos];const auto &animal=w.animals[id];auto &f=w.hollows;
    if(!animal.energy || !animal.body.limbs){out[pos]=r;return;}
    auto m=f.minds[id];r.subjects=1;Word k=mix(seed+id*2011);Word eligible=0;
    for(Word j=0;j<m.used;++j) {
        Word e=f.marks[Count(j)*n+id];Word cue=hollow::identity(e);
        if(hollow::strength(e) && (cue>>2)==hollow::harvest) eligible|=1u<<cue;
    }
    auto goal=hollow::recall(m,hollow::cells(f,id),eligible,turn,k);
    if(goal.slot==hollow::missing || goal.range<minimum){out[pos]=r;return;}
    r.goals=1;r.travel=goal.range;
    int dx=hollow::delta(goal.goal&255,m.x),dy=hollow::delta(goal.goal>>8,m.y);
    Word east=(animal.heading+hollow::effector(m,1))&3,south=(animal.heading+hollow::effector(m,2))&3;
    auto cx=[](Word d){return int(d==1)-int(d==3);};auto cy=[](Word d){return int(d==2)-int(d==0);};
    int x=dx*cx(east)+dy*cx(south),y=dx*cy(east)+dy*cy(south),spin=(k&1)?-1:1;
    auto locate=[&](int a,int b){return cell_at(Word(int(pos%c.side)+a),Word(int(pos/c.side)+b),c.side);};
    Word wanted=locate(x,y),other=locate(spin*y,-spin*x);
    const auto &a=f.beds[wanted],&b=f.beds[other];
    r.old_food=cut(a.carbon,16);r.other_food=cut(b.carbon,16);
    r.original_pits=bool(a.relief);r.other_pits=bool(b.relief);
    r.matching=a.relief && hollow::material(a)==(goal.cue&3);r.control_match=b.relief && hollow::material(b)==(goal.cue&3);
    r.grazed=cut(w.land[wanted].fruit,16);r.other_grazed=cut(w.land[other].fruit,16);
    if(!(id&15)) {
        Word source=f.sites[Count(goal.slot)*((n+15)/16)+id/16];
        if(source&0x80000000u){r.dated=1;Word place=(source&0x7fffffffu)-1;r.exact=wanted==place;r.control_exact=other==place;
            r.source_food=cut(f.beds[place].carbon,16);}
    }
    out[pos]=r;
}

KERNEL void effort_found(effort::Field f,Word n,Word seed){
    Word i=tid();if(i<n){f.minds[i]={effort::founder(mix(seed+i*3389))};f.beds[i]=0;}
}
KERNEL void effort_widen(unsigned char *next,const unsigned char *prior,Word side,Word old){
    Word i=tid();if(i<old*old)next[(i/old)*side+i%old]=prior[i];
}
KERNEL void effort_estuary(World w,Configuration c,Word tick,Word seed,Word mode,effort::Survey *out){
    Word pos=tid(),n=c.side*c.side;if(pos>=n)return;
    effort::Survey r{};Word id=w.slots[pos];auto a=w.animals[id];
    if(!a.energy || !a.body.limbs || !a.body.pores){out[pos]=r;return;}
    Word d=a.heading,other=neighbor(pos,d,c.side),edge=(d==1||d==2)?pos:other,axis=!(d&1);
    auto land=w.land[edge];Word wall=axis?land.south:land.east,weave=axis?land.fibre_south:land.fibre_east;
    Word mass=nbits(a.body.shape),depth=w.land[other].water/48,force=3+nbits(a.body.limbs)+((w.dna[id].grip>>5)&3);
    Word drag=1+(a.load+a.fibre)/4+mass/7+wall/2+depth-cut(depth,weave/(1+mass/4));
    Word tread=w.outlay.beds[edge],fee=crossing_fee(w,c,pos,edge,axis,tick,mass,wall,weave,tread);
    if(w.animals[w.slots[other]].energy || drag>force || a.energy<=fee+3){out[pos]=r;return;}
    auto here=w.hollows.beds[pos],there=w.hollows.beds[other];if(!here.relief&&!there.relief){out[pos]=r;return;}
    Word appetite=3+nbits(a.body.limbs)+((w.hollows.minds[id].genes.compass>>20)&3);
    Word own=cut(here.carbon,appetite),theirs=cut(there.carbon,appetite),bare=crossing_fee(w,c,pos,edge,axis,tick,mass,wall,weave,0);
    int local=int(own)-int(here.relief?1+(here.cover>>2):0),distant=int(theirs)-int(there.relief?1+(there.cover>>2):0)-int(fee);
    Word skin=stamped_skin(w,pos,axis,tread),target=fruit_skin(w,other),near=fruit_skin(w,pos);
    auto m=w.outlay.minds[id];
    if(mode==1){m.food={};m.work={};m.outcomes={};}
    if(mode==2)skin=(skin+1)&3;
    if(mode==3){auto other_m=w.outlay.minds[mix(seed+id)%n];m.work=other_m.work;}
    bool go=effort::elect(effort::expect(m,{near,skin,0}).charge,effort::expect(m,{target,skin,1}).charge,mix(seed+id*487));
    r.possible=1;r.picked=go;r.correct=(go?distant:local)>=(go?local:distant);
    r.better_here=local>distant;r.better_there=distant>local;r.correct_here=!go&&r.better_here;r.correct_there=go&&r.better_there;
    r.ties=local==distant;r.fruit=go?theirs:own;r.work=go?fee+Word(there.relief?1+(there.cover>>2):0):Word(here.relief?1+(here.cover>>2):0);
    r.regret=Word((local>distant?local:distant)-(go?distant:local));r.ordinary=fee;r.bare=bare;r.compacted=bare>fee;
    r.known=bool(m.experience&(1u<<near)) && bool(m.experience&(1u<<target)) && bool(m.experience&(16u<<skin));
    out[pos]=r;
}

KERNEL void effort_quay(World w,Configuration c,Word tick,Word ntrip,Word mode,effort::Paving *out){
    Word pos=tid();if(pos>=c.side*c.side)return;
    Word id=w.slots[pos];auto a=w.animals[id];auto &m=w.outlay.minds[id];effort::Paving r{};
    if(!a.energy || !a.body.limbs || w.land[pos].grains<2){out[pos]=r;return;}
    Word d=a.heading,opposite=neighbor(pos,d,c.side),edge=(d==1||d==2)?pos:opposite,axis=!(d&1),shift=4*axis;
    Word raw=w.outlay.beds[edge],packing=(raw>>shift)&15;if(packing==15){out[pos]=r;return;}
    auto bed=w.land[edge];Word wall=axis?bed.south:bed.east,fiber=axis?bed.fibre_south:bed.fibre_east,mass=nbits(a.body.shape);
    auto image=effort::freeze(m);if(mode==1)image.next=0;
    auto p=effort::prepare(image,stamped_skin(w,pos,axis,raw),ntrip,1+((m.genes.episode>>28)&1),mode==2);
    Word bill[3]{};
    for(Word attempt=0;attempt<3;++attempt){
        Word form=packing,work=0;
        for(Word j=0;j<attempt && form<15;++j){form+=cut(15-form,1+nbits(a.body.limbs)/3);++work;}
        for(Word j=0;j<ntrip;++j){
            Word tread=(raw&~(15u<<shift))|(form<<shift);
            work+=crossing_fee(w,c,(j&1)?opposite:pos,edge,axis,tick,mass,wall,fiber,tread);
            form+=cut(15-form,1+mass/12);
        }
        bill[attempt]=work;
    }
    Word best=cut(bill[0],cut(bill[1],bill[2]));r.plots=1;r.indicated=p.strokes!=0;r.tested=p.strokes;r.best=bill[p.strokes]==best;
    r.worth=best<bill[0];r.benefit=bill[p.strokes]<bill[0];r.harm=bill[p.strokes]>bill[0];r.work=bill[p.strokes];r.plain=bill[0];
    r.perfect=best;r.known=p.known;r.two=p.strokes==2;out[pos]=r;
}

#include "flood_linkages.cuh"
KERNEL void wick_tracers(wicks::Field field,Word n,Word seed,Word mint){
    Word i=tid();if(i>=n)return;bool marked=mint&&(mix(seed+i*7907)&31)==0;
    field.parcels[i]=marked?wicks::label(i,0):0;
    if(marked)count(field.ledger[n>=16384?i/256:0].tracers,1);
}
KERNEL void wick_rebed(wicks::Field field,Word n){Word i=tid();if(i<n)field.parcels[i]&=wicks::trail;}
KERNEL void wick_courses(Word n,Word length,Word ticks,Word mode,Word seed,wicks::Result *rows){
    Word global=tid(),sample=global>>7,i=global&127;if(sample>=n)return;
    __attribute__((shared)) static Word soil_storage[128*sizeof(Silt)/4],pore_storage[128*sizeof(hollow::Bed)/4],grain_storage[128*sizeof(material::Grain)/4];
    auto *ground=reinterpret_cast<Silt*>(soil_storage);auto *hollows=reinterpret_cast<hollow::Bed*>(pore_storage);auto *minerals=reinterpret_cast<material::Grain*>(grain_storage);
    __attribute__((shared)) static Word parcel[128];
    __attribute__((shared)) static Word water_caught,carbon_caught,work;
    Word k=mix(seed+sample*3797),water=4800+length*100,food=3200+length*24,mid=length/2-1;
    if(i<length){ground[i]={};ground[i].water=i?0:water;ground[i].litter=i?0:food;
        ground[i].fibre_east=i+1<length && (i!=mid||mode==0||mode==3)?1:0;
        hollows[i]={};minerals[i]={};minerals[i].faces=(k&1)?0x55:0xaa;parcel[i]=i?0:wicks::label(0,0);}
    if(!i)water_caught=carbon_caught=work=0;
    asm volatile("bar.sync 0;":::"memory");
    for(Word t=0;t<ticks;++t){
        if(t==ticks/2 && mode==2 && i==mid){ground[i].fibre_east=1;work=1;}
        asm volatile("bar.sync 0;":::"memory");
        if(i+1<length && (i&1)==(t&1) && !(mode==3 && (i&1))){
            auto flow=wicks::exchange(ground[i],ground[i+1],hollows[i],hollows[i+1],minerals[i],minerals[i+1],0,ground[i].fibre_east);
            if(flow.water)wicks::transport(parcel[i],parcel[i+1],flow.from_right);
        }
        asm volatile("bar.sync 0;":::"memory");
        if(i+1==length){water_caught+=ground[i].water;carbon_caught+=ground[i].litter;ground[i].water=ground[i].litter=0;}
        asm volatile("bar.sync 0;":::"memory");
    }
    if(!i){wicks::Result x{};x.n=1;x.length=length;x.ticks=ticks;x.water_before=water;x.food_before=food;x.water_caught=water_caught;x.food_caught=carbon_caught;
        x.strand_before=length-1;x.spare=mode==1;x.spent=work;
        for(Word j=0;j<length;++j){x.water_left+=ground[j].water;x.food_left+=ground[j].litter;x.strand_after+=ground[j].fibre_east;
            if(parcel[j]&wicks::present){x.distance=j;x.hops=wicks::hops(parcel[j]);}}
        rows[sample]=x;
    }
}

// A disjoint district gets exactly the same number of interventions in either arm.
// Adjacent sites on its boundary can be read here because only seam metadata changes.
KERNEL void reed_cut_sites(World w,Configuration c,Word salt,Word treatment,Word width,Word *pairs){
    Word block=tid(),wide=c.side/width;if(block>=wide*wide)return;
    auto f=w.wicks.reeds;Word count=2*width*width,k=mix(salt+block*5839),mask=count-1;
    unsigned char classes[2][32]{};Word matched=0;
    for(Word pass=0;pass<2;++pass){
        for(Word i=0;i<count;++i){
            Word order=(k+i*((k>>19)|1u))&mask,local=order>>1,axis=order&1;
            Word pos=(block/wide*width+local/width)*c.side+block%wide*width+local%width;
            Word to=axis?(pos+c.side)%(c.side*c.side):(pos%c.side+1==c.side?pos+1-c.side:pos+1);
            auto a=w.matter.bed[pos],b=w.matter.bed[to];Word shape=runnel::fold_at(f.seams[pos],axis);
            if(!shape||!a.faces||!b.faces)continue;
            Word cue=runnel::texture((a.faces>>(2*axis))&3,(b.faces>>(2*(axis+2)))&3);
            Word code=runnel::contact(f.stone,cue,shape-1),tag=runnel::traced(f.seams[pos],axis);
            Word context=code+4*axis+8*Word(w.land[pos].water>w.land[to].water)+16*Word((w.hollows.beds[pos].carbon|w.hollows.beds[to].carbon)!=0);
            auto &remaining=classes[tag][context];
            if(!pass)++remaining;
            else if(remaining){--remaining;if(treatment<2&&tag==treatment)runnel::set_fold(f.seams[pos],axis,0);}
        }
        if(!pass)for(Word k=0;k<32;++k){Word n=classes[0][k]<classes[1][k]?classes[0][k]:classes[1][k];classes[0][k]=classes[1][k]=n;matched+=n;}
    }
    pairs[block]=matched;
}
