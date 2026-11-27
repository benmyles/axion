#include "axion/actor.hpp"

using namespace axion;
using namespace axion::life;
#define ENTRY extern "C" __attribute__((global))
#define UNROLL _Pragma("unroll")

namespace {
AX_FN Word index() {
    Word block, thread, width;
    asm("mov.u32 %0, %%ctaid.x;" : "=r"(block));
    asm("mov.u32 %0, %%tid.x;" : "=r"(thread));
    asm("mov.u32 %0, %%ntid.x;" : "=r"(width));
    return block * width + thread;
}
AX_FN Word shuffle(Word x, Word source, Word mask) {
    Word result;
    asm volatile("shfl.sync.idx.b32 %0, %1, %2, 31, %3;" : "=r"(result) : "r"(x), "r"(source), "r"(mask));
    return result;
}
AX_FN Word lesser(Word a, Word b) { return a < b ? a : b; }

#include "somatic_genome.cuh"
AX_FN SynapticGenes inherit_synapse(SynapticGenes extra, const Genome &parent, Word key, bool mutate, Word &edits) {
    if (mutate) {
        Word chance = 4 + ((parent.reproduction >> 8) & 31);
        Word u = mix(key + 0xb253dc57u), v = mix(u + 713);
        if ((u & 255) < chance) { extra.conditional ^= 1u << ((u >> 8) & 31); ++edits; }
        if ((v & 255) < chance) { extra.modulation ^= 1u << ((v >> 8) & 31); ++edits; }
    }
    return extra;
}

AX_FN Word sky(Word pos, Word tick, Settings c, Word mask) {
    Word x = pos & mask, y = pos / c.side;
    Word patch = mix((x >> 3) + (y >> 3) * 0xc4ceb9f5u + c.seed);
    Word phase = ((tick >> 5) + (patch & 15)) & 15;
    Word light = phase < 8 ? phase : 15 - phase;
    Word noise = mix(pos ^ c.seed ^ (tick * 0x9e3779b9u));
    return (noise & 15) < (1 + light + ((patch >> 12) & 3)) ? 2u : 0u;
}

AX_FN Word territory(Word pos, Settings c);
AX_FN Chemistry microclimate(Word region,Word tick,Settings c) {
    Word interval=c.season ? c.season : 32;
    Word period=interval+((region>>17)%(1+interval/4));
    Word epoch=tick/period, offset=tick%period+(region&63);
    epoch+=offset/period; offset%=period;
    Word phase=mix(region^(epoch*0x9e3779b9u))&(c.climates-1);
    Word previous=mix(region^((epoch-1)*0x9e3779b9u))&(c.climates-1);
    return {phase,Word(offset<4 && offset<tick && phase!=previous)};
}
AX_FN Chemistry local_climate(Word position,Word tick,Settings c,const Chemistry *weather) {
    if (!c.climates) return {};
    if (!weather) return microclimate(territory(position,c),tick,c);
    Word across=c.side<16 ? 1 : c.side/16;
    Word region=position%c.side/16+(position/c.side/16)*across;
    return weather[(tick&31)*across*across+region];
}
AX_FN Word sense(const Site *world, Word pos, Word tick, Settings c) {
    Site here = world[pos];
    Word input = 0;
    UNROLL
    for (Word d = 0; d < 4; ++d) {
        Site other = world[neighbor(pos, d, c.side)];
        input |= Word(other.food > here.food) << d;
        input |= Word(other.body.energy != 0) << (4 + d);
        input |= Word(other.scent > here.scent) << (8 + d);
        input |= Word(other.body.energy > here.body.energy) << (12 + d);
    }
    input |= Word(here.food > 4) << 16 | Word(here.food > 20) << 17 | Word(here.food > 45) << 18;
    input |= Word(here.body.energy < 24) << 19 | Word(here.body.energy > 56) << 20;
    input |= Word(here.scent > 4) << 21 | Word(here.body.recent == 1) << 22 | Word(here.body.recent == 2) << 23;
    Word light=(tick>>5)&3;
    if (c.climate_cue) {
        Word region=territory(pos,c), state=microclimate(region,tick,c).low;
        light=(region+(region>>8|1)*state)&3;
    }
    input |= (1u << (24 + here.body.heading)) | (light << 28);
    input |= Word(here.stock > 0) << 30 | Word(here.stock > 12) << 31;
    return input;
}


#include "life_brain.cuh"
#include "outcomes.cuh"
#include "wayfinding.cuh"

// Each group of four fruit values uses every physical grip in a hidden
// permutation. Products in a compound grove carry two distinct signals.
AX_FN Word territory(Word pos, Settings c) {
    Word x = pos & (c.side - 1), y = pos / c.side;
    return mix(c.seed ^ (x >> 4) * 0x7b873593u ^ (y >> 4) * 0xcc9e2d51u);
}
AX_FN Word pod_receptor(Word region, Word species) {
    return ((region >> 16) + ((region >> 8) | 1) * species) & 31;
}
AX_FN Word attribute(Word key, Word value, Word bank) {
    return ((key + ((key >> 8) | 1) * value) & 15) + bank * 16;
}
AX_FN Word pair_cue(Word shape, Word texture) { return shape | (texture << 5) | 1024; }
AX_FN Word visible_pod(Word region, Word species, Settings c) {
    if (!paired_habitat(c.habitat)) return pod_receptor(region, species);
    return pair_cue(attribute(region, species & (c.species-1), 0), attribute(mix(region), species / c.species, 1));
}
AX_FN Word season_key(Word region, Word tick, Settings c) {
    Word period = c.season ? c.season + ((region >> 17) % (1+c.season/4)) : 1 + 18*c.species + ((region >> 17) & 63);
    return mix(region ^ (((tick + (region & 63)) / period) * 0x9e3779b9u));
}
AX_FN Word permutation(Word season, Word group) {
    Word key = (group ? mix(season ^ group*0x3c6ef375u) : season) % 24;
    Word order = 0xe4;
    UNROLL
    for (int i = 3; i > 0; --i) {
        Word j = key % (i+1); key /= i+1;
        Word diff = ((order >> (2*i)) ^ (order >> (2*j))) & 3;
        order ^= (diff << (2*i)) | (diff << (2*j));
    }
    return order;
}
AX_FN Word fruit_value(Word pod, bool texture, Settings c) {
    if (!paired_habitat(c.habitat)) return pod;
    return texture ? pod / c.species : pod & (c.species-1);
}
AX_FN Word grip(Word pos, Word tick, Word pod, Settings c, const Chemistry *weather) {
    if (weather) {
        Word across = c.side < 16 ? 1 : c.side/16;
        Word region = ((pos & (c.side-1)) >> 4) + (pos / c.side >> 4) * across;
        Chemistry entry = weather[(tick & 31)*across*across + region];
        if (c.habitat == Habitat::coupled) {
            Word a = pod & (c.species-1), b = pod / c.species;
            return ((entry.low >> (2*a)) + (entry.high >> (2*b))) & 3;
        }
        Word v = fruit_value(pod, entry.high >> 31, c);
        return ((v < 16 ? entry.low : entry.high) >> (2*(v & 15))) & 3;
    }
    Word season = season_key(territory(pos,c),tick,c);
    if (c.habitat == Habitat::coupled) {
        Word a = pod & (c.species-1), b = pod / c.species;
        Word p = permutation(mix(season ^ 0x23a786b9u),a >> 2);
        Word q = permutation(mix(season ^ 0xb6179f35u),b >> 2);
        return ((p >> (2*(a&3))) + (q >> (2*(b&3)))) & 3;
    }
    Word v = fruit_value(pod, season >> 31, c);
    return (permutation(season,v >> 2) >> (2*(v & 3))) & 3;
}
AX_FN Word plant_response(const Plant &plant, Word pod, Word species,Word reaction) {
    Word shape = (plant.shape >> (2*(pod & (species-1)))) & 3;
    Word texture = (plant.texture >> (2*(pod/species))) & 3;
    return (reaction >> (2*(4*shape+texture))) & 3;
}
AX_FN Word plant_grip(const Plant &p,Word pod,Word species) { return plant_response(p,pod,species,p.reaction); }
AX_FN Word surface_flavour(const Plant &plant,Word class_code,Word pins) {
    return course_switch(((pins&3)+class_code)&3,plant.seed)+1;
}
AX_FN Word material_code(Word chemistry,Word phase,Word flavour) {
    return (chemistry>>(8*phase+2*(flavour-1)))&3;
}
template<int Tiles, bool Grove, int Relational,bool Balanced=false,bool Navigation=false>
AX_FN void think(const Site *world, const Genome *genes, Tissue *brains, Intent *intent, Word tick, Settings c,
                 Word id, Word pos, Contacts *contacts, const unsigned char *pods, DelayLine *history, SynapticLayer *layers, AssociationLayer *links,OutcomeLayer *outcomes=nullptr) {
    Word tile = id & (Tiles - 1);
    Body body = world[pos].body;
    if (!body.energy) { if (!tile) intent[pos] = {}; return; }
    Word base = (id & 31) & ~(Tiles - 1);
    Word mask = Word((1ull << Tiles) - 1) << base;
    const Genome &g = genes[body.slot];
    Word sensory = 0;
    if (!tile) sensory = sense(world, pos, tick, c);
    sensory = shuffle(sensory, base, mask);
    Word demand=0,event=0;
    if constexpr (Balanced) {
        demand=appetite(outcomes[body.slot].stores,g.body);
        event=outcomes[body.slot].associations;
        sensory=food_field(sensory,demand,event);
    }
    if constexpr (Navigation) sensory^=landmark(pos,c);
    Word offset = body.slot * Tiles + tile;
    Tissue r = body.born == tick ? develop(mix(g.development + 0x9e3779b9u * tile)) : brains[offset];
    bool plastic = c.plastic && (c.contest != Contest::plasticity || treatment(body.founder, c));
    Word dendritic = 0, tag = 0;
    Intent summary{};
    MotorCounts votes{};
    Word tie = mix(tick * 0x9e3779b9u + body.slot + c.seed);
    Word map_origin=0; MapOutput navigation{};
    if constexpr (Navigation) {
        auto &storage=map_tail(outcomes,c.side*c.side)[body.slot];
        MapCircuit<Tiles> circuit; bool fresh=body.born==tick;
        circuit.load(storage,tile,fresh);circuit.load_trace(outcomes,c.side*c.side,body.slot,tile,fresh,c.navigation);
        if (!treatment(body.founder,c)) {
            if (c.contest==Contest::navigation_signal) circuit.cut=MapCut::silent;
            if (c.contest==Contest::navigation_plasticity) circuit.cut=MapCut::fixed;
            if (c.contest==Contest::navigation_range) circuit.cut=MapCut::one_pulse;
            if (c.contest==Contest::temporal_signal) circuit.cut=MapCut::raw;
            if (c.contest==Contest::temporal_plasticity) circuit.cut=MapCut::trace_fixed;
            if (c.contest==Contest::temporal_span) circuit.cut=MapCut::immediate;
        }
        Word observation=landmark(pos,c);
        if (!fresh && plastic) circuit.encounter(storage.previous,observation,absorbed(event),tile);
        navigation=circuit.output(g,observation,0,demand,tile,base,mask,c.navigation,tie);
        circuit.sense_event(observation,tile,base,mask);circuit.store_trace(outcomes,c.side*c.side,body.slot,tile);
        circuit.store(storage,tile); map_origin=circuit.address(observation);
    }
    if constexpr (Grove) {
        tag = visible_pod(territory(pos, c), pods[pos], c);
        sensory ^= pattern(tag);
        if constexpr (Relational==3) {
            summary=neural<Tiles,true>(g,r,sensory,tile,base,mask,plastic);
            votes=motor_counts<Tiles>(g,r,tile,base,mask);
        }
        auto &record = contacts[body.slot];
        Dendrites<Tiles,Relational> synapses;
        bool fresh = body.born == tick;
        auto *layer = layers ? &layers[body.slot] : nullptr;
        auto *connections = Relational ? &links[body.slot] : nullptr;
        auto *contexts = Relational==3 ? context_tail(links,c.side*c.side)+body.slot : nullptr;
        auto *expression=Relational==3 && c.programs==2 ? expression_tail(links,c.side*c.side)+body.slot : nullptr;
        synapses.load(g, &record, tile, fresh, layer, connections,contexts,expression,c.polarity);
        if constexpr (Relational) if (c.contest == Contest::recurrent_paths && !treatment(body.founder,c))
            synapses.control = PathControl::direct_only;
        if constexpr (Relational >= 2) if (c.contest == Contest::distal_paths && !treatment(body.founder,c))
            synapses.control = PathControl::short_range;
        if constexpr (Relational==3) if (!treatment(body.founder,c)) {
            if (c.contest==Contest::contextual_memory) synapses.context_cut=ContextCut::pooled;
            if (c.contest==Contest::contextual_evidence) synapses.context_cut=ContextCut::blind;
        }
        Word last = 0;
        if (!tile && !fresh) {
            if (history) {
                Word lag = conduction(g);
                if (tick - body.born >= lag) last = history[body.slot].discharge[(tick - lag) & 7];
            } else last = record.previous;
        }
        last = shuffle(last, base, mask);
        bool adjustable = c.contest != Contest::dendritic_plasticity || treatment(body.founder, c);
        bool adjust_slow = c.contest != Contest::metaplasticity || treatment(body.founder, c);
        Word stamp=0;
        if (plastic && adjustable) synapses.update(g, g.expression >> 16, last, body.recent == 1, tile, adjust_slow,history ? conduction(g) : 1);
        dendritic = synapses.fire(g, tag, tile, base, mask,false,nullptr,sensory ^ pattern(tag),&stamp,&votes,tie);
        synapses.remember(stamp);
        if constexpr (Relational==3) synapses.store_context(*contexts,tile,expression);
        if constexpr (Relational) {
            if (fresh || (plastic && adjustable && last)) {
                synapses.store(record,tile,layer);
                synapses.persist_event(*connections,tile,last,body.recent == 1,fresh);
            }
        } else synapses.store(record,tile,layer);
    }
    if constexpr (Relational!=3) {
        summary=neural<Tiles,true>(g,r,sensory,tile,base,mask,plastic);
        votes=motor_counts<Tiles>(g,r,tile,base,mask);
    }
    if constexpr (Balanced) {
        auto &stock=outcomes[body.slot]; bool fresh=body.born==tick;
        Prospects<Tiles> view;
        view.load(stock,tile,fresh);
        if (!treatment(body.founder,c)) {
            if (c.contest==Contest::prospective) view.cut=OutcomeCut::silent;
            if (c.contest==Contest::prospective_plasticity) view.cut=OutcomeCut::fixed;
            if (c.contest==Contest::outcome_association) view.cut=OutcomeCut::no_association;
        }
        Word issued=contacts[body.slot].previous;
        if (!fresh && plastic && issued) view.observe(g,pattern(earlier_cue(issued)),(issued>>5)&7,tasted(event),absorbed(event),tile);
        dendritic=view.drive(g,pattern(tag),demand,dendritic,tile,base,mask);
        view.store(stock,tile);
    }
    if constexpr (Navigation) dendritic=navigation.apply(dendritic,&votes);
    brains[offset] = r;
    bool random_motor = c.random_actions || (c.contest == Contest::agency && !treatment(body.founder, c));
    summary.action = random_motor ? tie & 7 : select_motor<Grove>(g,votes,tie,dendritic);
    if (!tile) {
        intent[pos] = summary;
        if constexpr (Navigation) map_tail(outcomes,c.side*c.side)[body.slot].previous=0x2000u|(Word(effector(g,summary.action))<<5)|map_origin;
        if constexpr (Grove) {
            Word issued = delayed(tag, summary.action);
            contacts[body.slot].previous = issued;
            if (history) history[body.slot].discharge[tick & 7] = static_cast<unsigned short>(issued);
        }
    }
}
// Occupied-site scheduling uses one ballot for a spatial strip of 32 sites.
// Subgroups pick the next living bodies directly from that mask. Empty sites
// use no tissue lanes and the schedule needs no global list or prefix scan.
template<int Tiles, bool Grove, bool Packed, int Relational = 0,bool Balanced=false,bool Navigation=false>
AX_FN void schedule(const Site *world, const Genome *genes, Tissue *brains, Intent *intent, const Word *epoch,
                    Word offset, Settings c, Contacts *contacts, const unsigned char *pods, DelayLine *history, SynapticLayer *layers, AssociationLayer *links=nullptr,OutcomeLayer *outcomes=nullptr) {
    Word id = index(), tick = epoch ? *epoch + offset : offset;
    if constexpr (!Packed) {
        Word pos = id / Tiles;
        if (pos < c.side*c.side) think<Tiles,Grove,Relational,Balanced,Navigation>(world,genes,brains,intent,tick,c,id,pos,contacts,pods,history,layers,links,outcomes);
    } else {
        if (id >= c.side*c.side) return;
        Word occupied, alive = world[id].body.energy != 0;
        asm volatile("{ .reg .pred p; setp.ne.u32 p, %1, 0; vote.sync.ballot.b32 %0, p, -1; }"
                     : "=r"(occupied) : "r"(alive));
        Word size = population(occupied);
        for (Word rank = (id & 31)/Tiles; rank < size; rank += 32/Tiles) {
            Word position;
            asm("fns.b32 %0, %1, 0, %2;" : "=r"(position) : "r"(occupied), "r"(rank+1));
            Word pos = (id & ~31u) + position;
            think<Tiles,Grove,Relational,Balanced,Navigation>(world,genes,brains,intent,tick,c,id,pos,contacts,pods,history,layers,links,outcomes);
        }
    }
}

struct Observation { Word field, cue; };
template<bool Grove>
AX_FN Observation sample_senses(const Site *world, const unsigned char *pods, Word focal, Word key, Word tick, Settings c) {
    Word pos = focal;
    for (Word draw = 0; draw < 8; ++draw) {
        Word candidate = mix(key + draw) & (c.side*c.side - 1);
        if (world[candidate].body.energy) { pos = candidate; break; }
    }
    Word cue = 0;
    if constexpr (Grove) cue = visible_pod(territory(pos, c), pods[pos], c);
    return {sense(world, pos, tick, c), cue};
}
template<int Tiles, bool Grove, int Relational>
AX_FN Word replay_observation(const Genome &g, Tissue &state, Dendrites<Tiles,Relational> &synapses, PeripheralHistory &history,
                              Observation view, Word tie, Word lag, Word tile, Word base, Word mask,
                              bool plastic, bool change_contacts, bool adjust_slow, bool random_motor, bool record) {
    Word sensory = view.field, drive = 0;
    if constexpr (Grove) sensory ^= pattern(view.cue);
    neural<Tiles, false>(g, state, sensory, tile, base, mask, plastic);
    if (!Grove && record) return 0;
    auto votes=motor_counts<Tiles>(g,state,tile,base,mask);
    if constexpr (Grove) {
        // At the common test only the two acquired sets of gates and weights
        // differ. Pending sensory/motor history is not transplanted.
        if (change_contacts && record)
            synapses.update(g, g.expression >> 16, history.at(lag), (view.field >> 22) & 1, tile, adjust_slow,lag);
        Word stamp=0;
        drive = synapses.fire(g, view.cue, tile, base, mask,false,nullptr,view.field,&stamp,&votes,tie);
        if (record) synapses.remember(stamp);
    }
    Word action = random_motor ? tie & 7 : select_motor<Grove>(g,votes,tie,drive);
    if constexpr (Grove) if (record) history.append(delayed(view.cue, action));
    return action;
}
template<int Tiles, bool Grove, int Relational = 0>
AX_FN void assay(const Site *world, const Genome *genes, const Tissue *brains, Assay *results,
                 Word tick, Word repeats, Word control, Settings c, const Contacts *contacts = nullptr,
                 const unsigned char *pods = nullptr, const DelayLine *retained = nullptr, const SynapticLayer *layers = nullptr, const AssociationLayer *links = nullptr) {
    Word id = index(), pos = id / Tiles, tile = id & (Tiles - 1);
    if (pos >= c.side*c.side) return;
    Body body = world[pos].body;
    if (!body.energy) return;
    Word base = (id & 31) & ~(Tiles - 1), mask = Word((1ull << Tiles) - 1) << base;
    const Genome &g = genes[body.slot];
    bool live = tile < living_tiles(Tiles, g.development);
    bool cohort = treatment(body.founder, c);
    bool plastic = c.plastic && (c.contest != Contest::plasticity || cohort);
    bool dendritic = plastic && (c.contest != Contest::dendritic_plasticity || cohort);
    bool slow = c.contest != Contest::metaplasticity || cohort;
    bool random_motor = c.random_actions || (c.contest == Contest::agency && !cohort);
    bool fresh = body.born == tick;
    Tissue initial = fresh ? develop(mix(g.development + 0x9e3779b9u * tile)) : brains[body.slot*Tiles + tile];
    Dendrites<Tiles,Relational> initial_banks{};
    PeripheralHistory prior{};
    Word lag = 1;
    if constexpr (Grove) {
        initial_banks.load(g,&contacts[body.slot],tile,fresh,layers ? &layers[body.slot] : nullptr,Relational ? &links[body.slot] : nullptr,
                           Relational==3 ? context_tail(links,c.side*c.side)+body.slot : nullptr,
                           Relational==3 && c.programs==2 ? expression_tail(links,c.side*c.side)+body.slot : nullptr,c.polarity);
        if constexpr (Relational) if (c.contest == Contest::recurrent_paths && !cohort)
            initial_banks.control = PathControl::direct_only;
        if constexpr (Relational >= 2) if (c.contest == Contest::distal_paths && !cohort)
            initial_banks.control = PathControl::short_range;
        if constexpr (Relational==3) if (!cohort) {
            if (c.contest==Contest::contextual_memory) initial_banks.context_cut=ContextCut::pooled;
            if (c.contest==Contest::contextual_evidence) initial_banks.context_cut=ContextCut::blind;
        }
        if (retained) {
            lag = conduction(g);
            Word age = lesser(tick - body.born, 8);
            for (Word k = age; k; --k) prior.append(retained[body.slot].discharge[(tick - k) & 7]);
        } else if (!fresh) prior.append(contacts[body.slot].previous);
    }
    Assay count{};
    for (Word probe = 0; probe < repeats; ++probe) {
        Tissue a = initial, b = initial;
        Dendrites<Tiles,Relational> sa = initial_banks, sb = initial_banks;
        PeripheralHistory ha = prior, hb = prior;
        Word key = trial_key(c.seed ^ 0xb834d127u, pos, probe);
        for (Word frame = 0; frame < 3+lag; ++frame) {
            Observation first{}, second{};
            if (!tile) {
                Word draw = mix(key + 2 * frame);
                first = sample_senses<Grove>(world, pods, pos, draw, tick, c);
                second = control == 1 ? first : sample_senses<Grove>(world, pods, pos, mix(draw + 1), tick, c);
            }
            first.field = shuffle(first.field, base, mask); second.field = shuffle(second.field, base, mask);
            if constexpr (Grove) { first.cue = shuffle(first.cue, base, mask); second.cue = shuffle(second.cue, base, mask); }
            Word tie = mix(key ^ frame);
            replay_observation<Tiles,Grove>(g,a,sa,ha,first,tie,lag,tile,base,mask,control != 2 && plastic,control != 2 && dendritic,slow,random_motor,true);
            replay_observation<Tiles,Grove>(g,b,sb,hb,second,tie,lag,tile,base,mask,control != 2 && plastic,control != 2 && dendritic,slow,random_motor,true);
        }
        Word differs = live ? (a.gate_a ^ b.gate_a) | (a.gate_b ^ b.gate_b) : 0;
        if constexpr (Grove) {
            UNROLL
            for (int i = 0; i < sa.words; ++i) if (tile+i*Tiles < 8) differs |= (sa.bank[i] ^ sb.bank[i]) | (sa.slow[i] ^ sb.slow[i]);
        }
        if constexpr (Relational) differs |= sa.different(sb,tile);
        if constexpr (Relational==3) {
            differs |= sa.context_differs(sb,tile);
            sb.activity=sa.activity; sb.proposals=sa.proposals; sb.landmarks=sa.landmarks;
            sb.program_history=sa.program_history;
        }
        count.gates += group_sum<Tiles>(differs != 0, tile, base, mask) != 0;
        b.spike = a.spike; b.memory = a.memory;
        Observation query{};
        if (!tile) query = sample_senses<Grove>(world, pods, pos, mix(key ^ 0x96e2cd71u), tick, c);
        query.field = shuffle(query.field, base, mask);
        if constexpr (Grove) query.cue = shuffle(query.cue, base, mask);
        Word left = replay_observation<Tiles,Grove>(g,a,sa,ha,query,key,lag,tile,base,mask,plastic,false,slow,random_motor,false);
        Word right = replay_observation<Tiles,Grove>(g,b,sb,hb,query,key,lag,tile,base,mask,plastic,false,slow,random_motor,false);
        count.spikes += group_sum<Tiles>(live && a.spike != b.spike, tile, base, mask) != 0;
        count.motors += left != right;
        count.effectors += effector(g,left) != effector(g,right);
    }
    if (!tile) {
        auto &r = results[control];
#define ASSAY_ADD(field, n) asm volatile("red.global.add.u32 [%0], %1;" :: "l"(&r.field), "r"(n) : "memory")
        ASSAY_ADD(trials, repeats); ASSAY_ADD(gates, count.gates); ASSAY_ADD(spikes, count.spikes);
        ASSAY_ADD(motors, count.motors); ASSAY_ADD(effectors, count.effectors);
#undef ASSAY_ADD
    }
}

template<class T>
struct PatchPlane {
    T (*data)[128];
    Word lane;
    AX_FN T &operator[](int offset) { return data[offset][lane]; }
};

AX_FN int inside(int k, unsigned dir) {
    if (dir == 0 && (k & 2)) return k ^ 2;
    if (dir == 1 && !(k & 1)) return k ^ 1;
    if (dir == 2 && !(k & 2)) return k ^ 2;
    if (dir == 3 && (k & 1)) return k ^ 1;
    return -1;
}
AX_FN Word plant_fruit(Plant p, Word draw, Word species) {
    // Eight inherited bits set the typical appearance. Eight independently
    // mutable switches admit developmental variation in the two populations.
    Word variation = p.seed >> 24;
    Word appearance = mux(variation,draw,p.seed >> 16);
    return (appearance & (species-1)) + ((appearance >> 4) & (species-1))*species;
}
} // namespace

#if defined(AX_FAMILY_life)
// A stratified sample keeps long runs responsive to crowding without a full census.
ENTRY void life_occupancy(const Site *world,Word sites,Word key,Word *result) {
    Word thread=index(),stride=sites/4096,occupied=0;
    for (Word sample=thread;sample<4096;sample+=256)
        occupied+=world[sample*stride+(mix(key^sample)&(stride-1))].body.energy!=0;
    Word sum;
    asm("redux.sync.add.u32 %0, %1, 0xffffffff;" : "=r"(sum) : "r"(occupied));
    __attribute__((shared)) Word warps[8];
    if (!(thread&31)) warps[thread/32]=sum;
    asm volatile("bar.sync 0;" ::: "memory");
    if (!thread) {
        Word total=0;
        UNROLL
        for (int i=0;i<8;++i) total+=warps[i];
        *result=total;
    }
}
template<int Level>
AX_FN void renew_contacts(const Site *sites,const Genome *genes,Contacts *contacts,SynapticLayer *layers,
                          AssociationLayer *links,Settings c,Word tick,OutcomeLayer *outcomes) {
    Word pos=index()/8, tile=index()&7;
    if (pos>=c.side*c.side) return;
    const Body &body=sites[pos].body;
    if (!body.energy || body.born==tick) return;
    Word slot=body.slot;
    auto *slow=layers ? layers+slot : nullptr;
    auto *link=Level ? links+slot : nullptr;
    auto *context=Level==3 ? context_tail(links,c.side*c.side)+slot : nullptr;
    Dendrites<8,Level> newborn;
    newborn.load(genes[slot],nullptr,tile,true,slow,link,context);
    newborn.store(contacts[slot],tile,slow,link);
    // Preserve activity, delayed credit, recruitment, genomes and metabolism.
    if constexpr (Level==3) if (tile<4) context->cells[tile]=newborn.cell[0];
    if (outcomes) {
        Prospects<8> naive; naive.load(outcomes[slot],tile,true); naive.store(outcomes[slot],tile);
        if (c.navigation>=4) {
            auto &s=trace_tail(outcomes,c.side*c.side)[slot];
            for (Word i=tile;i<32;i+=8) {s.predictions[i]=0;s.sources[i]=0;}
            if (!tile) {s.conflicts=0;s.held=0;s.recent=0;}
        }
        if (c.navigation) { auto &map=map_tail(outcomes,c.side*c.side)[slot]; MapCircuit<8> fresh; fresh.load(map,tile,true);fresh.store(map,tile); }
    }
}
#define RENEW(level) ENTRY void life_renew##level(const Site *s,const Genome *g,Contacts *c,SynapticLayer *d,AssociationLayer *a,Settings p,Word t,OutcomeLayer *o) { renew_contacts<level>(s,g,c,d,a,p,t,o); }
RENEW(0)
RENEW(1)
RENEW(2)
RENEW(3)
#undef RENEW
ENTRY void life_init(Site *world, Genome *genes, Settings c, unsigned char *pods, SynapticLayer *layers, AssociationLayer *links, Plant *flora,OutcomeLayer *outcomes,Word plant_seed) {
    Word pos = index();
    if (pos >= c.side * c.side) return;
    Word energy = founding_energy(pos, c.seed, c.density);
    Word draw = mix(pos ^ c.seed);
    world[pos] = {{pos, pos, 0, 0, static_cast<unsigned short>(energy), static_cast<unsigned char>(draw & 3), 0},
                  static_cast<unsigned char>(initial_food(pos, c.seed)), 0, 0};
    Genome g = founder(mix(draw ^ 0x935a42efu));
    if (c.habitat != Habitat::meadow) {
        pods[pos] = mix(draw + 1) & ((paired_habitat(c.habitat) ? c.species*c.species : c.species) - 1);
        // All founders start with exact identity transmission at the plastic
        // junction. An experienced acquisition rule must arise by mutation.
        g.expression = (g.expression & 0xffffu) | 0xaaaa0000u;
        g.edges[1] |= 31u << 10;
        if (links) links[pos] = {link_founder(draw),{}, {}, 0};
        if (c.relations==3) context_tail(links,c.side*c.side)[pos]={context_founder(draw),{},0,0,0};
        if (c.programs==2) expression_tail(links,c.side*c.side)[pos]={{context_founder(draw),mix(draw+83),mix(draw+89)},0};
        if (outcomes) outcomes[pos]={outcome_founder(draw),{},0,0};
        if (c.navigation) map_tail(outcomes,c.side*c.side)[pos]={map_founder(draw),{},{},0};
        if (c.navigation>=4) trace_tail(outcomes,c.side*c.side)[pos]={trace_founder(draw),{},{},0,0,0};
        if (flora) {
            Word patch=((pos&(c.side-1))/c.stand)+(pos/c.side/c.stand)*(c.side/c.stand);
            Word k = mix(plant_seed ^ (patch*0x7694acab) ^ 0xd7e86b45u);
            flora[pos] = {mix(k),mix(k+1),mix(k+2),mix(k+3)};
            for (Word phase=1;phase<c.climates;++phase) {
                Word noise=mix(k+phase*0x632be5abu), reaction=noise;
                if (noise & 1) {
                    Word code=permutation(mix(noise),0); reaction=0;
                    UNROLL
                    for (Word j=0;j<16;++j) {
                        Word grip=(flora[pos].reaction>>(2*j))&3;
                        reaction |= ((code>>(2*grip))&3)<<(2*j);
                    }
                }
                Word sites=c.side*c.side;
                reinterpret_cast<Word *>(flora+sites)[(phase-1)*sites+pos]=reaction;
            }
            if (outcomes) {
                Word identity=0;
                UNROLL
                for (Word phase=0;phase<4;++phase) identity|=permutation(mix(k^0x22ed66d5u^phase*0xb5297a4du),0)<<(8*phase);
                auto *extra=reinterpret_cast<Word *>(flora+c.side*c.side);
                extra[(c.climates ? c.climates-1 : 0)*c.side*c.side+pos]=identity;
            }
            if (c.fruit == Fruit::inherited) pods[pos] = plant_fruit(flora[pos],mix(draw+1),c.species);
        }
        if (layers) {
            // The optional register and the primary weight both initially copy
            // themselves. Mutations can realize any four-state local transition.
            layers[pos] = {{0x000aaaaau, 0xffff0000u}, {}};
        }
        // Four opposable grips are initially available. Every effector byte
        // retains the same point mutation access as in the original habitat.
        Word offset = draw >> 30;
        UNROLL
        for (Word a = 0; a < 8; ++a) {
            Word shift = 8 * (a & 3);
            Word code = ((effector(g, a) & ~3u) | 8 | ((a + offset) & 3));
            g.effectors[a >> 2] = (g.effectors[a >> 2] & ~(255u << shift)) | (code << shift);
        }
    }
    genes[pos] = g;
}
#define BRAIN_ARGS const Site *w, const Genome *g, Tissue *b, Intent *a, const Word *epoch, Word offset, Settings c
#define GROVE_ARGS Contacts *contacts, const unsigned char *pods, DelayLine *history, SynapticLayer *layers
#define REL_THINK(n, prefix, level) \
    ENTRY void prefix##_think##n(BRAIN_ARGS, GROVE_ARGS, AssociationLayer *links) { schedule<n,true,false,level>(w,g,b,a,epoch,offset,c,contacts,pods,history,layers,links); } \
    ENTRY void prefix##_packed##n(BRAIN_ARGS, GROVE_ARGS, AssociationLayer *links) { schedule<n,true,true,level>(w,g,b,a,epoch,offset,c,contacts,pods,history,layers,links); }
#define THINK(n) \
    ENTRY void life_think##n(BRAIN_ARGS) { schedule<n,false,false>(w,g,b,a,epoch,offset,c,nullptr,nullptr,nullptr,nullptr); } \
    ENTRY void life_packed##n(BRAIN_ARGS) { schedule<n,false,true>(w,g,b,a,epoch,offset,c,nullptr,nullptr,nullptr,nullptr); } \
    ENTRY void grove_think##n(BRAIN_ARGS, GROVE_ARGS) { schedule<n,true,false>(w,g,b,a,epoch,offset,c,contacts,pods,history,layers); } \
    ENTRY void grove_packed##n(BRAIN_ARGS, GROVE_ARGS) { schedule<n,true,true>(w,g,b,a,epoch,offset,c,contacts,pods,history,layers); } \
    REL_THINK(n, linked, 1) \
    REL_THINK(n, wave, 2) \
    REL_THINK(n, context, 3)
THINK(2)
THINK(4)
THINK(8)
THINK(16)
THINK(32)
#endif
#if defined(AX_FAMILY_assays)
#define REL_ASSAY(n, prefix, level) \
    ENTRY void prefix##_assay##n(const Site *w, const Genome *g, const Tissue *b, Assay *out, Word tick, Word repeats, Word control, Settings c, const Contacts *contacts, const unsigned char *pods, const DelayLine *history, const SynapticLayer *layers, const AssociationLayer *links) { assay<n,true,level>(w, g, b, out, tick, repeats, control, c, contacts, pods, history, layers, links); }
#define ASSAY(n) ENTRY void life_assay##n(const Site *w, const Genome *g, const Tissue *b, Assay *out, Word tick, Word repeats, Word control, Settings c) { assay<n, false>(w, g, b, out, tick, repeats, control, c); } \
    ENTRY void grove_assay##n(const Site *w, const Genome *g, const Tissue *b, Assay *out, Word tick, Word repeats, Word control, Settings c, const Contacts *contacts, const unsigned char *pods, const DelayLine *history, const SynapticLayer *layers) { assay<n, true>(w, g, b, out, tick, repeats, control, c, contacts, pods, history, layers); } \
    REL_ASSAY(n, linked, 1) \
    REL_ASSAY(n, wave, 2) \
    REL_ASSAY(n, context, 3)
ASSAY(2)
ASSAY(4)
ASSAY(8)
ASSAY(16)
ASSAY(32)
#endif

#if defined(AX_FAMILY_life)
// A partition belongs to one CUDA thread until this tick is over. Seeds
// transfer stored biomass, and neither genomes nor energy need atomics.
AX_FN void disperse(Plant *flora, SeedLedger &history, unsigned char *pods, Word x, Word y, PatchPlane<Site> q, Word key, Settings c) {
    if (c.flora == Flora::fixed) return;
    Word source = key & 3, dest = source ^ (1u << ((key >> 2) & 1));
    Word parent_pos = cell_at(x+(source&1),y+(source>>1),c.side);
    Plant p = flora[parent_pos];
    Word mass = 1 + (p.seed & 7), surplus = 4 + ((p.seed >> 3) & 15);
    Word parent = q[source].food, ground = q[dest].food;
    if (parent < ground+surplus || parent < mass || ground+mass > 80 ||
        ((key >> 3) & 255) > ((p.seed >> 7) & 255)) return;
    q[source].food -= mass; q[dest].food += mass;
    Word draw = mix(key ^ 0x2ca6f457u);
    Word edit=~0u;
    if (((c.mutate && c.flora==Flora::evolving) || c.flora==Flora::autonomous) && (draw & 255) < ((p.seed >> 16) & 127)) {
        Word bit = 1u << ((draw >> 8) & 31);
        Word chemistries=c.climates ? c.climates : 1;
        edit=c.balanced ? (draw>>13)%(4+chemistries) : c.climates>1 ? (draw>>13)%(3+c.climates) : (draw>>13)&3;
        switch (edit) {
            case 0: p.shape ^= bit; break;
            case 1: p.texture ^= bit; break;
            case 2: p.reaction ^= bit; break;
            case 3: p.seed ^= bit; break;
        }
        ++history.edits;
    }
    Word new_site = cell_at(x+(dest&1),y+(dest>>1),c.side);
    for (Word phase=1;phase<c.climates;++phase) {
        Word sites=c.side*c.side;
        auto *plane=reinterpret_cast<Word *>(flora+sites)+(phase-1)*sites;
        plane[new_site]=plane[parent_pos] ^ (edit==3+phase ? 1u<<((draw>>8)&31) : 0);
    }
    if (c.balanced) {
        auto *food=food_chemistry(flora,c.side*c.side,c.climates);
        food[new_site]=food[parent_pos] ^ (edit==3+(c.climates ? c.climates : 1) ? 1u<<((draw>>8)&31) : 0);
    }
    flora[new_site] = p;
    if (c.fruit == Fruit::inherited) pods[new_site] = plant_fruit(p,mix(key+0xec0071u),c.species);
    ++history.births;
}
template<class T> AX_FN void exchange(T &left, T &right) {
    T old = left; left = right; right = old;
}
template<bool Grove, bool Plants = false,bool Balanced=false>
AX_FN void settle_patch(const Site *world, Site *next, Genome *genes, const Intent *intent, Ledger *records,
                        const Word *epoch, Word offset, Settings c, unsigned char *pods, Harvest *harvest, DelayLine *history, SynapticLayer *layers, const Chemistry *weather, AssociationLayer *links,
                        Plant *flora = nullptr, SeedLedger *propagation = nullptr,OutcomeLayer *outcomes=nullptr) {
    Word tick = epoch ? *epoch + offset : offset;
    Word block = index(), cells = c.side * c.side;
    if (block >= cells / 4) return;
    Word mask = c.side - 1, half = c.side / 2;
    // All four offsets recur. The source snapshot is immutable for this tick.
    Word x = 2 * (block & (half - 1)) + (tick & 1), y = 2 * (block / half) + ((tick ^ (tick >> 1)) & 1);
    Word key = trial_key(c.seed, block, tick);
    // Each lane owns a different neighborhood. Placing the four dynamically
    // addressed occupants in shared memory avoids a private main-memory stack.
    __attribute__((shared)) Site occupants[4][128];
    __attribute__((shared)) Word slots[4][128], traits[4][128], actions[4][128];
    Word lane = block & 127, pos[4];
    PatchPlane<Site> q{occupants, lane};
    PatchPlane<Word> slot{slots, lane}, trait{traits, lane}, act{actions, lane};
    Ledger l = records[block];
    Word attempts = 0, opened = 0, climate_tries=0, climate_opened=0;
    Word intake_low=0,intake_high=0,choices=0,converted=0,seep_low=0,seep_high=0,leached=0;
    UNROLL
    for (int k = 0; k < 4; ++k) {
        pos[k] = cell_at(x + (k & 1), y + (k >> 1), c.side);
        q[k] = world[pos[k]];
        Body &b = q[k].body;
        slot[k] = b.slot;
        Intent a = b.energy ? intent[pos[k]] : Intent{};
        trait[k] = 0; act[k] = 0;
        if (b.energy) {
            trait[k] = genes[b.slot].body;
            act[k] = effector(genes[b.slot], a.action);
        }
        l.updates += a.updates; l.spikes += a.spikes; l.gate_changes += a.changed;
        Word growth = lesser(80 - q[k].food, sky(pos[k], tick, c, mask));
        q[k].food += growth; l.growth += growth;
        if (q[k].stock && q[k].food < 80 && ((tick + pos[k]) & 15) == 0) {
            --q[k].stock; ++q[k].food;
        }
        unsigned sum = 4 * q[k].scent;
        UNROLL
        for (int d = 0; d < 4; ++d) sum += world[neighbor(pos[k], d, c.side)].scent;
        q[k].scent = static_cast<unsigned char>((sum * 15) >> 7);
        if (!b.energy) continue;
        if constexpr (Balanced) outcomes[b.slot].associations&=0xff00ffffu;
        if constexpr (Grove) {
            if (c.digestion > 1) {
                auto &gut = history[b.slot].digestion;
                Word packet = gut[tick & 7];
                gut[tick & 7] = 0;
                if (packet == 255) {
                    Word loss = lesser(2, b.energy);
                    b.energy -= loss; l.spent += loss;
                    if (!b.energy) { ++l.deaths; continue; }
                } else {
                    Word uptake = lesser(packet, 1023 - b.energy);
                    b.energy += uptake; l.spent += packet - uptake;
                }
            }
        }
        // Locomotion, signaling and maintaining a large neural tissue consume energy.
        Word ports = act[k];
        Word effort = Word(population(ports & 0x5cu));
        Word cost = 1 + Word((ports & 4) && ((tick + b.slot) & 3) == 0)
                    + Word((tick & 7) == 0 && a.updates > 512) + (effort > 1 ? effort - 1 : 0);
        Word paid = lesser(b.energy, cost);
        b.energy -= paid; l.spent += paid;
        if (!b.energy) { ++l.deaths; continue; }
        bool feed = ports & 8;
        Word waiting=0,following=0;
        if constexpr (Balanced) {
            auto &organ=outcomes[b.slot];
            waiting=chewing(organ.stores);
            organ.stores=contents(organ.stores);
            following=waiting ? waiting-1 : 0;
        }
        Word flavour=0,nutrient=0,water=0;
        if constexpr (Balanced) if (c.springs) water=aquifer(pos[k],local_climate(pos[k],tick,c,weather).low,c);
        if constexpr (Grove) {
            if (feed && q[k].food) {
                Word species = pods[pos[k]];
                Word unlock;
                Word early=0;
                if constexpr (Plants) {
                    Word position=pos[k], reaction=flora[position].reaction, phase=0;
                    if (c.climates) {
                        Chemistry sky=local_climate(position,tick,c,weather);
                        if (sky.low) reaction=reinterpret_cast<const Word *>(flora+cells)[(sky.low-1)*cells+position];
                        phase=sky.low;
                        early=sky.high;
                    }
                    unlock=plant_response(flora[position],species,c.species,reaction);
                    if constexpr (Balanced) {
                        flavour=surface_flavour(flora[position],unlock,ports);
                        nutrient=material_code(food_chemistry(flora,cells,c.climates)[position],phase,flavour);
                    }
                }
                else unlock = grip(pos[k], tick, species, c, weather);
                if constexpr (Balanced) {
                    auto &p=outcomes[b.slot];
                    p.associations=receipt(p.associations,flavour,0);
                    bool mineral=!c.springs || water==nutrient+1;
                    choices+=1+(Word(mineral && ((appetite(p.stores,trait[k])>>nutrient)&1))<<16);
                    feed=!waiting && ((mineral && pouch_space(p.stores,nutrient,trait[k])) || (c.balanced==2 && (p.associations>>24)<255));
                } else feed=(ports&3)==unlock;
                ++attempts;
                opened += feed;
                if constexpr (Plants) if (c.climates) {
                    Word bucket=2*Word(c.contest && treatment(b.founder,c))+early;
                    climate_tries+=1u<<(8*bucket); climate_opened+=Word(feed)<<(8*bucket);
                }
                if (!Plants || feed || c.fruit == Fruit::scattered) {
                    Word fruit = mix(key + k * 797u);
                    if constexpr (Plants) {
                        if (c.fruit == Fruit::inherited) fruit = plant_fruit(flora[pos[k]],fruit,c.species);
                    }
                    pods[pos[k]] = fruit & ((paired_habitat(c.habitat) ? c.species*c.species : c.species) - 1);
                }
                if (!Balanced && !feed && c.digestion > 1) {
                    history[b.slot].digestion[(tick + c.digestion - 1) & 7] = 255;
                } else if (!Balanced && !feed) {
                    Word friction = lesser(2, b.energy);
                    b.energy -= friction; l.spent += friction;
                    if (!b.energy) { ++l.deaths; continue; }
                }
            }
        }
        if (feed) {
            Word space=1023-b.energy;
            if constexpr (Balanced) {
                space=!c.springs || water==nutrient+1 ? pouch_space(outcomes[b.slot].stores,nutrient,trait[k]) : 0;
                Word carbon_space=255-(outcomes[b.slot].associations>>24);
                if (c.balanced==2 && carbon_space>space) space=carbon_space;
            }
            Word size=Balanced ? mouthful(trait[k],c.husk) : 7+(trait[k]&15);
            Word gain=lesser(lesser(q[k].food,size),space);
            q[k].food -= gain; l.grazed += gain;
            if constexpr (Balanced) {
                if (gain) {
                    following=handling_time(gain,c.husk);
                    auto &p=outcomes[b.slot];
                    bool mineral=!c.springs || water==nutrient+1;
                    Word minerals=mineral ? lesser(gain,pouch_space(p.stores,nutrient,trait[k])) : 0;
                    p.stores+=minerals<<(8*nutrient);
                    if (c.balanced==2) {
                        Word usable=lesser(gain,255-(p.associations>>24));
                        p.associations+=usable<<24; l.spent+=gain-usable;
                    }
                    p.associations=receipt(p.associations,flavour,mineral ? nutrient+1 : 0);
                    Word amount=gain<<(16*(nutrient&1));
                    if (nutrient<2) intake_low+=amount; else intake_high+=amount;
                }
            } else if constexpr (Grove) {
                if (c.digestion > 1)
                    history[b.slot].digestion[(tick + c.digestion - 1) & 7] = static_cast<unsigned char>(gain);
                else b.energy += gain;
            } else b.energy += gain;
        }
        if constexpr (Balanced) {
            auto &p=outcomes[b.slot];
            if (water) {
                Word kind=water-1,gain=lesser(c.springs,pouch_space(p.stores,kind,trait[k]));
                p.stores+=gain<<(8*kind);
                if (gain) p.associations=receipt(p.associations,tasted(p.associations),water);
                if (kind<2) seep_low+=gain<<(16*kind); else seep_high+=gain<<(16*(kind-2));
            }
            if ((c.seep && q[k].food) || c.leach) {
                Word phase=local_climate(pos[k],tick,c,weather).low;
                Word region=territory(pos[k],c), limiting=(region+((region>>11)|1)*phase)&3;
                Word loss=lesser(c.leach,(p.stores>>(8*limiting))&255);
                p.stores-=loss<<(8*limiting); leached+=loss;
                Word cap=16+((trait[k]>>2)&31),start=(key>>(2*k))&3;
                UNROLL
                for (Word i=0;i<4;++i) {
                    Word kind=(start+i)&3, held=(p.stores>>(8*kind))&255;
                    Word uptake=kind==limiting || held>=cap ? 0 : lesser(lesser(c.seep,cap-held),q[k].food);
                    p.stores+=uptake<<(8*kind);
                    if (c.balanced!=2) q[k].food-=uptake;
                    if (kind<2) seep_low+=uptake<<(16*kind); else seep_high+=uptake<<(16*(kind-2));
                }
            }
            Word carbon=p.associations>>24;
            Word energy=release_energy(p.stores,trait[k],1023-b.energy,carbon,c.balanced==2); converted+=energy;
            p.stores|=following<<30;
            p.associations=(p.associations&0xffffffu)|(carbon<<24);
            if (c.digestion>1) history[b.slot].digestion[(tick+c.digestion-1)&7]=static_cast<unsigned char>(energy);
            else b.energy+=energy;
        }
        if (ports & 64) {
            q[k].scent = static_cast<unsigned char>(lesser(255, q[k].scent + 12 + ((trait[k] >> 20) & 31)));
            ++l.marks;
        }
        if (ports & 0x14) b.heading = ports & 3;
        if ((ports & 0x90) == 0x90) {
            if (ports & 32) {
                Word give = lesser(lesser(b.energy - 1, 2 + ((trait[k] >> 16) & 7)), 1023 - q[k].stock);
                q[k].stock += give; b.energy -= give; l.stored += give;
            } else {
                Word take = lesser(lesser(q[k].stock, 5 + (15 - (trait[k] & 15))), 1023 - b.energy);
                b.energy += take; q[k].stock -= take; l.retrieved += take;
            }
        }
    }
    // All contacts are between bodies from the start of the tick. The order of
    // contested contacts varies per neighborhood; a dead body never acts again.
    for (int i = 0; i < 4; ++i) {
        int k = (key + i * ((key & 4) ? 3 : 1)) & 3;
        Body &a = q[k].body;
        int j = inside(k, a.heading);
        if (!a.energy || j < 0 || !q[j].body.energy || (act[k] & 0x90) != 0x10) continue;
        Body &b = q[j].body;
        if (!(act[k] & 32)) {
            Word power = 5 + (15 - (trait[k] & 15)), armor = (trait[j] >> 8) & 7;
            Word gain = lesser(lesser(b.energy, power > armor ? power - armor : 1), 1023 - a.energy);
            b.energy -= gain; a.energy += gain; l.bitten += gain;
            if (!b.energy) ++l.deaths;
        } else {
            Word gift = lesser(lesser(a.energy - 1, 2 + ((trait[k] >> 16) & 7)), 1023 - b.energy);
            a.energy -= gift; b.energy += gift; l.donated += gift;
        }
    }
    UNROLL
    for (int k = 0; k < 4; ++k) {
        Body &b = q[k].body;
        if (b.energy) {
            int delta = int(b.energy) - int(world[pos[k]].body.energy);
            b.recent = delta > 0 ? 1 : (delta < 0 ? 2 : 0);
        } else if constexpr (Grove) {
            if constexpr (Balanced) if (world[pos[k]].body.energy) {
                Word material=contents(outcomes[b.slot].stores), mass=0;
                UNROLL
                for (Word j=0;j<4;++j) mass+=(material>>(8*j))&255;
                if (c.balanced==2) mass=outcomes[b.slot].associations>>24;
                Word returned=lesser(mass,80-q[k].food);
                q[k].food+=returned; l.spent+=mass-returned; outcomes[b.slot].stores=0;
                outcomes[b.slot].associations&=0x00ffffffu;
            }
            if (history && world[pos[k]].body.energy) {
                Word remnant = 0;
                UNROLL
                for (int j = 0; j < 8; ++j) {
                    auto &packet = history[b.slot].digestion[j];
                    remnant += packet == 255 ? 0 : packet;
                    packet = 0;
                }
                Word stock = lesser(remnant, 80 - q[k].food);
                q[k].food += stock; l.spent += remnant - stock;
            }
        }
    }
    for (int i = 0; i < 4; ++i) {
        int k = ((key >> 8) + i * ((key & 8) ? 3 : 1)) & 3;
        Body &a = q[k].body;
        if (a.slot != slot[k] || !a.energy || !(act[k] & 4)) continue;
        int j = inside(k, a.heading);
        if (j < 0 || q[j].body.energy) continue;
        Body &b = q[j].body;
        // The two records live in shared memory. Exchanging scalar fields
        // avoids a thread-local 20-byte temporary materialized by Clang.
        exchange(a.slot,b.slot); exchange(a.founder,b.founder); exchange(a.generation,b.generation);
        exchange(a.born,b.born); exchange(a.energy,b.energy); exchange(a.heading,b.heading); exchange(a.recent,b.recent);
        ++l.moves;
    }
    for (int i = 0; i < 4; ++i) {
        int k = ((key >> 16) + i) & 3;
        Body &a = q[k].body;
        if (a.energy < 64 || a.born == tick + 1) continue;
        const Genome &parent = genes[a.slot];
        constexpr Word regions=2+Word(Grove)+Word(Balanced);
        Word cost=fidelity_cost(parent.reproduction,regions,c.repair);
        if (a.energy<64+(parent.reproduction&63)+cost) continue;
        int j = k ^ ((key & (1u << (20 + k))) ? 1 : 2);
        if (q[j].body.energy) j ^= 3;
        if (q[j].body.energy) continue;
        a.energy-=cost; l.spent+=cost;
        Word free_slot = q[j].body.slot;
        Word split = a.energy * (3 + ((parent.reproduction >> 16) & 3)) / 10;
        a.energy -= split;
        q[j].body = {free_slot, a.founder, a.generation + 1, tick + 1, static_cast<unsigned short>(split),
                     static_cast<unsigned char>((key >> (2 * j)) & 3), 0};
        Word edits = 0, structural = 0;
        Word variation=mutable_regions(parent.reproduction,key+k,regions,c.repair);
        bool synaptic_change=c.mutate && (variation&4);
        inherit(parent,genes[free_slot],mix(key+k),c.mutate,edits,structural,variation);
        if constexpr (Balanced) {
            auto &mother=outcomes[a.slot],&child=outcomes[free_slot];
            child.genes=inherit_outcomes(mother.genes,parent,key+k,c.mutate && (variation&8),edits);
            if (c.navigation) {
                auto *maps=map_tail(outcomes,cells);
                if (c.navigation>=4) { auto *trace=trace_tail(outcomes,cells);trace[free_slot].genes=inherit_trace(trace[a.slot].genes,parent,key+k,c.mutate && (variation&8),edits); }
                maps[free_slot].genes=inherit_map(maps[a.slot].genes,parent,mother.genes,key+k,c.mutate && (variation&8),edits);
            }
            Word share=divide_stocks(mother.stores,parent.reproduction);
            Word substrate=(mother.associations>>24)*(3+((parent.reproduction>>16)&3))/10;
            mother.stores-=share; child.stores=share; child.associations=substrate<<24; mother.associations-=substrate<<24;
        }
        if constexpr (Grove) {
            if (links) {
                Word change = 4+((parent.reproduction>>8)&31);
                links[free_slot].genes = inherit_links(links[a.slot].genes,key+k,change,synaptic_change,edits);
                links[free_slot].convergence = inherit_convergence(links[a.slot].convergence,key+k,change,synaptic_change,edits);
                if (c.relations==3) {
                    auto *contexts=context_tail(links,cells);
                    contexts[free_slot].genes=inherit_context(contexts[a.slot].genes,parent,links[a.slot].genes,key+k,change,synaptic_change,edits);
                    if (c.programs==2) {
                        auto *expression=expression_tail(links,cells);
                        expression[free_slot].genes=inherit_expression(expression[a.slot].genes,contexts[a.slot].genes,parent,
                            links[a.slot].genes,key+k,change,synaptic_change,edits);
                    }
                }
            }
            if (layers) {
                layers[free_slot].genes = inherit_synapse(layers[a.slot].genes, parent, key+k, synaptic_change, edits);
            }
        }
        l.structural += structural;
        l.edits += edits; ++l.births;
    }
    if constexpr (Plants) disperse(flora,propagation[block],pods,x,y,q,mix(key ^ 0x1072c6b9u),c);
    UNROLL
    for (int k = 0; k < 4; ++k) next[pos[k]] = q[k];
    records[block] = l;
    if constexpr (Grove) {
        harvest[block].attempts += attempts;
        harvest[block].opened += opened;
    }
    if constexpr (Plants) if (c.climates) {
        Word active, tries, wins;
        asm("activemask.b32 %0;" : "=r"(active));
        asm("redux.sync.add.u32 %0, %1, %2;" : "=r"(tries) : "r"(climate_tries),"r"(active));
        asm("redux.sync.add.u32 %0, %1, %2;" : "=r"(wins) : "r"(climate_opened),"r"(active));
        if (!(block&31)) {
            auto &shard=reinterpret_cast<ClimateLedger *>(harvest+cells/4)[block/32];
            UNROLL
            for (Word k=0;k<4;++k) {
                shard.group[k].attempts+=(tries>>(8*k))&255;
                shard.group[k].opened+=(wins>>(8*k))&255;
            }
        }
    }
    if constexpr (Balanced) {
        Word mask, lo,hi,meals,burn;
        asm("activemask.b32 %0;" : "=r"(mask));
        asm("redux.sync.add.u32 %0, %1, %2;" : "=r"(lo) : "r"(intake_low),"r"(mask));
        asm("redux.sync.add.u32 %0, %1, %2;" : "=r"(hi) : "r"(intake_high),"r"(mask));
        asm("redux.sync.add.u32 %0, %1, %2;" : "=r"(meals) : "r"(choices),"r"(mask));
        asm("redux.sync.add.u32 %0, %1, %2;" : "=r"(burn) : "r"(converted),"r"(mask));
        if (c.seep || c.springs) {
            Word a,b;
            asm("redux.sync.add.u32 %0, %1, %2;" : "=r"(a) : "r"(seep_low),"r"(mask));
            asm("redux.sync.add.u32 %0, %1, %2;" : "=r"(b) : "r"(seep_high),"r"(mask));
            if (!(block&31)) {
                auto *table=reinterpret_cast<SeepLedger *>(diet_ledger(harvest,cells,c.climates)+climate_shards(cells));
                auto &l=table[block/32]; l.uptake[0]+=a&65535; l.uptake[1]+=a>>16; l.uptake[2]+=b&65535; l.uptake[3]+=b>>16;
            }
        }
        if (c.leach) {
            Word lost; asm("redux.sync.add.u32 %0, %1, %2;" : "=r"(lost) : "r"(leached),"r"(mask));
            if (!(block&31)) {
                auto *end=reinterpret_cast<SeepLedger *>(diet_ledger(harvest,cells,c.climates)+climate_shards(cells));
                reinterpret_cast<Count *>(end+((c.seep || c.springs) ? climate_shards(cells) : 0))[block/32]+=lost;
            }
        }
        if (!(block&31)) {
            auto &ledger=diet_ledger(harvest,cells,c.climates)[block/32];
            ledger.intake[0]+=lo&65535; ledger.intake[1]+=lo>>16; ledger.intake[2]+=hi&65535; ledger.intake[3]+=hi>>16;
            ledger.tasted+=meals&65535; ledger.depleted+=meals>>16; ledger.converted+=burn;
        }
    }
}
#define SETTLE_ARGS const Site *world, Site *next, Genome *genes, const Intent *intent, Ledger *records, const Word *epoch, Word offset, Settings c
ENTRY void life_settle(SETTLE_ARGS) { settle_patch<false>(world, next, genes, intent, records, epoch, offset, c, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr); }
ENTRY void grove_settle(SETTLE_ARGS, unsigned char *pods, Harvest *harvest, DelayLine *history, SynapticLayer *layers, const Chemistry *weather, AssociationLayer *links) {
    settle_patch<true>(world, next, genes, intent, records, epoch, offset, c, pods, harvest, history, layers, weather, links);
}
ENTRY void thicket_settle(SETTLE_ARGS, unsigned char *pods, Harvest *harvest, DelayLine *history, SynapticLayer *layers, const Chemistry *weather, AssociationLayer *links, Plant *flora, SeedLedger *propagation) {
    settle_patch<true,true>(world,next,genes,intent,records,epoch,offset,c,pods,harvest,history,layers,weather,links,flora,propagation);
}
ENTRY void food_settle(SETTLE_ARGS,unsigned char *pods,Harvest *harvest,DelayLine *history,SynapticLayer *layers,const Chemistry *weather,AssociationLayer *links,Plant *flora,SeedLedger *propagation,OutcomeLayer *outcomes) {
    settle_patch<true,true,true>(world,next,genes,intent,records,epoch,offset,c,pods,harvest,history,layers,weather,links,flora,propagation,outcomes);
}
#undef SETTLE_ARGS

// A graph batch shares at most 32 upcoming chemistry records per territory.
ENTRY void life_weather(Chemistry *table, const Word *epoch, Settings c) {
    Word id = index(), across = c.side < 16 ? 1 : c.side/16, regions = across*across;
    if (id >= 32*regions) return;
    Word region = id & (regions-1), pos = (region%across)*16 + (region/across)*16*c.side;
    Word time = *epoch + id/regions;
    if (c.habitat==Habitat::thicket) {
        table[(time&31)*regions+region]=microclimate(territory(pos,c),time,c);
        return;
    }
    Word season = season_key(territory(pos,c),time,c);
    Chemistry entry{};
    if (c.habitat == Habitat::coupled) {
        Word left = mix(season ^ 0x23a786b9u), right = mix(season ^ 0xb6179f35u);
        for (Word g = 0; g < c.species/4; ++g) {
            entry.low |= permutation(left,g) << (8*g);
            entry.high |= permutation(right,g) << (8*g);
        }
    } else {
        for (Word g = 0; g < c.species/4; ++g) {
            Word packed = permutation(season,g) << (8*(g & 3));
            if (g < 4) entry.low |= packed; else entry.high |= packed;
        }
        if (c.habitat == Habitat::compound) entry.high |= season & 0x80000000u;
    }
    table[(time & 31)*regions + region] = entry;
}

ENTRY void life_clock(Word *epoch, Word ticks) { *epoch += ticks; }

namespace {
AX_FN void add_group(Count &field, Count value, Word lane) {
    UNROLL
    for (int d = 16; d; d /= 2) {
        Word lo = shuffle(Word(value), lane ^ d, ~0u);
        Word hi = shuffle(Word(value >> 32), lane ^ d, ~0u);
        value += (Count(hi) << 32) | lo;
    }
    if (!lane) asm volatile("red.global.add.u64 [%0], %1;" :: "l"(&field), "l"(value) : "memory");
}
}
ENTRY void life_census(const Site *world, const Genome *genes, const Tissue *brains, const Ledger *records,
                       Census *total, Word *families, Settings c, const Contacts *contacts, const unsigned char *pods, Harvest *harvest, const DelayLine *history, const SynapticLayer *layers, const AssociationLayer *links, Plant *flora, const SeedLedger *propagation,const OutcomeLayer *outcomes,Word tick,Word profile) {
    Word pos = index(), cells = c.side * c.side, lane = pos & 31;
    // The grid is a power-of-two square with at least 64 sites.
    if (pos >= cells) return;
    Site q = world[pos];
    Body b = q.body;
    Ledger l{};
    if (pos < cells / 4) l = records[pos];
#define FIELD(f) add_group(total->history.f, l.f, lane)
    FIELD(births); FIELD(deaths); FIELD(moves); FIELD(bitten); FIELD(donated); FIELD(marks);
    FIELD(edits); FIELD(growth); FIELD(spent); FIELD(grazed); FIELD(updates); FIELD(spikes); FIELD(gate_changes); FIELD(structural); FIELD(stored); FIELD(retrieved);
#undef FIELD
    add_group(total->alive, b.energy > 0, lane);
    add_group(total->cohort, b.energy && treatment(b.founder, c), lane);
    add_group(total->tape_sum, b.energy ? 1 + (genes[b.slot].expression & 7) : 0, lane);
    add_group(total->reserve, b.energy, lane);
    if (profile) {
        Word available=0;
        UNROLL
        for (Word m=0;m<8;++m) { Word pins=effector(genes[b.slot],m); if (pins&8) available|=1u<<(pins&3); }
        UNROLL
        for (Word k=0;k<4;++k) add_group(total->fidelity[k],b.energy ? (genes[b.slot].reproduction>>(20+3*k))&7 : 0,lane);
        add_group(total->grip_count,b.energy ? population(available) : 0,lane);
        add_group(total->full_grips,b.energy && available==15,lane);
        add_group(total->age,b.energy ? tick-b.born : 0,lane);
        add_group(total->seasons_lived,b.energy && tick-b.born>=(c.season ? c.season : 32),lane);
    }
    add_group(total->plants, q.food, lane);
    add_group(total->caches, q.stock, lane);
    add_group(total->depth_sum, b.energy ? b.generation : 0, lane);
    add_group(total->founders_energy, initial_food(pos, c.seed) + founding_energy(pos, c.seed, c.density), lane);
    if (b.energy) {
        asm volatile("red.global.or.b32 [%0], %1;" :: "l"(families + b.founder / 32), "r"(1u << (b.founder & 31)) : "memory");
        asm volatile("red.global.max.u32 [%0], %1;" :: "l"(&total->deepest), "r"(b.generation) : "memory");
    }
    Word hash = mix(pos ^ c.seed);
    struct SiteWords { Word data[6]; };
    auto site_words = __builtin_bit_cast(SiteWords, q);
    UNROLL
    for (int i = 0; i < 6; ++i) hash = mix(hash ^ site_words.data[i]);
    auto g = __builtin_bit_cast(Chromosome, genes[pos]);
    UNROLL
    for (int i = 0; i < 18; ++i) hash = mix(hash ^ g.bases[i]);
    for (Word j = 0; j < c.tiles; ++j) {
        Tissue t = brains[pos * c.tiles + j];
        hash = mix(hash ^ t.spike ^ rotate(t.memory, 9));
        hash = mix(hash ^ t.gate_a ^ rotate(t.gate_b, 17));
    }
    if (c.habitat != Habitat::meadow) {
        Harvest h{};
        if (pos < cells/4) h = harvest[pos];
        add_group(total->harvest.attempts, h.attempts, lane);
        add_group(total->harvest.opened, h.opened, lane);
        auto &memory = contacts[pos];
        hash = mix(hash ^ pods[pos] ^ (memory.previous << 2));
        UNROLL
        for (int i = 0; i < 8; ++i) hash = mix(hash ^ memory.weights[i]);
    }
    if (links) {
        const auto &a = links[pos];
        hash = mix(hash ^ a.genes.switches); hash = mix(hash ^ a.genes.plastic); hash = mix(hash ^ a.genes.wiring);
        for (int r=0;r<16;++r) { hash = mix(hash ^ a.rows[r][0]); hash = mix(hash ^ a.rows[r][1]); }
        for (Word bits : a.recalled) hash = mix(hash ^ bits);
        hash = mix(hash ^ a.convergence);
        if (c.relations==3) {
            const auto &s=context_tail(links,cells)[pos]; auto p=s.genes;
            hash=mix(hash^p.plastic); hash=mix(hash^p.evidence); hash=mix(hash^p.dynamics);
            hash=mix(hash^p.coordination); hash=mix(hash^p.receptors);
            for (const auto &cell:s.cells) {
                for (Word inputs:cell.inputs) hash=mix(hash^inputs);
                hash=mix(hash^cell.bindings);
            }
            hash=mix(hash^s.activity); hash=mix(hash^s.proposals); hash=mix(hash^s.landmarks);
            if (c.programs==2) {
                const auto &ex=expression_tail(links,cells)[pos]; auto gene=ex.genes.alternative;
                hash=mix(hash^gene.plastic); hash=mix(hash^gene.evidence); hash=mix(hash^gene.dynamics);
                hash=mix(hash^gene.coordination); hash=mix(hash^gene.receptors);
                hash=mix(hash^ex.genes.sensors); hash=mix(hash^ex.genes.gate); hash=mix(hash^ex.history);
            }
        }
    }
    if (flora) {
        Plant p = flora[pos];
        hash = mix(hash ^ p.shape); hash = mix(hash ^ p.texture); hash = mix(hash ^ p.reaction); hash = mix(hash ^ p.seed);
        Word boundaries = 0, menus = 0;
        UNROLL
        for (Word direction=1; direction<=2; ++direction) {
            Plant q = flora[neighbor(pos,direction,c.side)];
            Word different = (p.shape ^ q.shape) | (p.texture ^ q.texture) | (p.reaction ^ q.reaction);
            boundaries += (different | (p.seed ^ q.seed)) != 0;
            Word cue = mix((2*pos+direction) ^ c.seed) & (c.species*c.species-1);
            menus += plant_grip(p,cue,c.species) != plant_grip(q,cue,c.species);
        }
        SeedLedger seeds{};
        if (pos < cells/4) seeds = propagation[pos];
        add_group(total->propagation.births,seeds.births,lane); add_group(total->propagation.edits,seeds.edits,lane);
        add_group(total->plant_boundaries,boundaries,lane); add_group(total->chemistries,menus,lane);
        if (c.climates) {
            const auto *alternates=reinterpret_cast<const Word *>(flora+cells);
            Word cue=mix(pos^c.seed^0x9d378c41u)&(c.species*c.species-1), differences=0;
            for (Word phase=1;phase<c.climates;++phase) {
                Word reaction=alternates[(phase-1)*cells+pos];
                hash=mix(hash^reaction);
                differences+=plant_grip(p,cue,c.species)!=plant_response(p,cue,c.species,reaction);
            }
            add_group(total->climate_differences,differences,lane);
            ClimateLedger ledger{};
            if (pos<climate_shards(cells)) ledger=reinterpret_cast<const ClimateLedger *>(harvest+cells/4)[pos];
            UNROLL
            for (Word k=0;k<4;++k) {
                add_group(total->climate.group[k].attempts,ledger.group[k].attempts,lane);
                add_group(total->climate.group[k].opened,ledger.group[k].opened,lane);
            }
        }
    }
    if (outcomes) {
        const auto &m=outcomes[pos];
        if (c.navigation) {
            const auto &map=map_tail(outcomes,cells)[pos];
            hash=mix(hash^map.genes.plastic);hash=mix(hash^map.genes.wiring);hash=mix(hash^map.genes.spread);hash=mix(hash^map.genes.currents);
            for (Word row:map.contacts) hash=mix(hash^row);
            for (Word row:map.appetites) hash=mix(hash^row);
            hash=mix(hash^map.previous);
            if (c.navigation>=4) {
                const auto &state=trace_tail(outcomes,cells)[pos];
                hash=mix(hash^state.genes.plastic);hash=mix(hash^state.genes.charge);hash=mix(hash^state.genes.attention);
                for (Word k=0;k<32;++k) hash=mix(hash^state.predictions[k]^state.sources[k]);
                hash=mix(hash^state.recent);hash=mix(hash^state.held);hash=mix(hash^state.conflicts);
            }
        }
        add_group(total->carbon,m.associations>>24,lane);
        hash=mix(hash^m.genes.plastic); hash=mix(hash^m.genes.wiring); hash=mix(hash^m.genes.valuation); hash=mix(hash^m.genes.gates);
        for (Word row:m.afferents) hash=mix(hash^row);
        hash=mix(hash^m.stores); hash=mix(hash^m.associations); hash=mix(hash^food_chemistry(flora,cells,c.climates)[pos]);
        if (c.seep || c.springs) {
            auto *table=reinterpret_cast<const SeepLedger *>(diet_ledger(harvest,cells,c.climates)+climate_shards(cells));
            SeepLedger seep{}; if (pos<climate_shards(cells)) seep=table[pos];
            UNROLL
            for (Word k=0;k<4;++k) add_group(total->seepage.uptake[k],seep.uptake[k],lane);
        }
        if (c.leach) {
            auto *end=reinterpret_cast<const SeepLedger *>(diet_ledger(harvest,cells,c.climates)+climate_shards(cells));
            auto *loss=reinterpret_cast<const Count *>(end+((c.seep || c.springs) ? climate_shards(cells) : 0));
            add_group(total->leached,pos<climate_shards(cells) ? loss[pos] : 0,lane);
        }
        Word recipes=food_chemistry(flora,cells,c.climates)[pos], current=local_climate(pos,tick,c,nullptr).low, present=0,ever=0;
        for (Word phase=0;phase<(c.climates ? c.climates : 1);++phase) {
            Word mask=0;
            UNROLL
            for (Word f=0;f<4;++f) mask|=1u<<((recipes>>(8*phase+2*f))&3);
            ever|=mask; if (phase==current) present=mask;
        }
        UNROLL
        for (Word k=0;k<4;++k) {
            add_group(total->supply[k],(present>>k)&1,lane); add_group(total->cycle_supply[k],(ever>>k)&1,lane);
        }
        DietLedger stats{};
        if (pos<climate_shards(cells)) stats=diet_ledger(harvest,cells,c.climates)[pos];
        UNROLL
        for (Word k=0;k<4;++k) {
            add_group(total->stocks[k],(contents(m.stores)>>(8*k))&255,lane);
            add_group(total->diet.intake[k],stats.intake[k],lane);
        }
        add_group(total->diet.tasted,stats.tasted,lane); add_group(total->diet.depleted,stats.depleted,lane); add_group(total->diet.converted,stats.converted,lane);
    }
    if (layers) {
        const auto &extra = layers[pos];
        hash = mix(hash ^ extra.genes.conditional); hash = mix(hash ^ extra.genes.modulation);
        UNROLL
        for (int j = 0; j < 8; ++j) hash = mix(hash ^ extra.state[j]);
    }
    if (history) {
        Word pending = 0;
        UNROLL
        for (int i = 0; i < 8; ++i) {
            Word packet = history[pos].digestion[i];
            pending += packet == 255 ? 0 : packet;
            hash = mix(hash ^ history[pos].discharge[i] ^ (packet << 16));
        }
        add_group(total->digesting, pending, lane);
    }
    asm volatile("redux.sync.xor.b32 %0, %1, -1;" : "=r"(hash) : "r"(hash));
    if (!lane) asm volatile("red.global.xor.b32 [%0], %1;" :: "l"(&total->fingerprint), "r"(hash) : "memory");
}

// Compact exceptionally sparse donor populations in a stable spatial order.
// One coalesced read per site avoids a separate allocator or a large prefix tree.
template<bool Positions> AX_FN void compact_living(const Site *source, Word size, Word *slots) {
    Word lane = index(), offset = 0;
    for (Word i = 0; i < size; i += 32) {
        const Body &b = source[i + lane].body;
        Word present, alive = b.energy != 0;
        asm volatile("{ .reg .pred p; setp.ne.u32 p, %1, 0; vote.sync.ballot.b32 %0, p, -1; }"
                     : "=r"(present) : "r"(alive));
        if (alive) slots[offset + population(present & ((1u << lane) - 1))] = Positions ? i+lane : b.slot;
        offset += population(present);
    }
}
ENTRY void life_compact(const Site *source, Word size, Word *slots) { compact_living<false>(source,size,slots); }
ENTRY void life_positions(const Site *source, Word size, Word *positions) { compact_living<true>(source,size,positions); }
// A warp owns 512 consecutive sites. The grid first counts its stripes, then
// scatters each living slot in exactly the same order as the scalar warp scan.
template<bool Scatter,bool Positions>
AX_FN void census_stripes(const Site *source,Word stripes,Word *offsets,Word *output) {
    Word thread=index(),stripe=thread>>5,lane=thread&31;
    if (stripe>=stripes) return;
    Word offset=Scatter ? offsets[stripe] : 0;
    _Pragma("unroll 2")
    for (Word base=stripe*512;base<(stripe+1)*512;base+=32) {
        const Body &b=source[base+lane].body;
        Word present,alive=b.energy!=0;
        asm volatile("{ .reg .pred p; setp.ne.u32 p, %1, 0; vote.sync.ballot.b32 %0, p, -1; }"
                     : "=r"(present) : "r"(alive));
        if constexpr (Scatter) if (alive) output[offset+population(present&((1u<<lane)-1))]=Positions ? base+lane : b.slot;
        offset+=population(present);
    }
    if constexpr (!Scatter) if (!lane) offsets[stripe]=offset;
}
ENTRY void life_stripes(const Site *s,Word n,Word *offsets,Word *out) { census_stripes<false,false>(s,n,offsets,out); }
ENTRY void life_index_slots(const Site *s,Word n,Word *offsets,Word *out) { census_stripes<true,false>(s,n,offsets,out); }
ENTRY void life_index_sites(const Site *s,Word n,Word *offsets,Word *out) { census_stripes<true,true>(s,n,offsets,out); }
ENTRY void life_stripe_offsets(Word *offsets,Word count) {
    Word lane=index(),carry=0;
    for (Word base=0;base<count;base+=32) {
        Word i=base+lane, value=i<count ? offsets[i] : 0, sum=value;
        UNROLL
        for (Word d=1;d<32;d*=2) {
            Word other=shuffle(sum,(lane-d)&31,~0u);
            if (lane>=d) sum+=other;
        }
        if (i<count) offsets[i]=carry+sum-value;
        carry+=shuffle(sum,31,~0u);
    }
}
ENTRY void dispersal_collect(const Site *world,const Word *positions,const Genome *genomes,const SynapticLayer *dendrites,
                            const AssociationLayer *links,const OutcomeLayer *outcomes,Settings c,
                            Word survivors,Word count,Word seed,Propagule *pool) {
    Word i=index();
    if (i>=count) return;
    // A random cyclic offset and disjoint strata sample residents without replacement.
    Word rank=(Word(Count(i)*survivors/count)+mix(seed)%survivors)%survivors;
    const Body &parent=world[positions[rank]].body;
    Word slot=parent.slot,sites=c.side*c.side;
    auto &p=pool[i];
    p.genome=genomes[slot]; p.ancestor=parent.founder; p.generation=parent.generation;
    if (dendrites) p.dendrites=dendrites[slot].genes;
    if (links) p.paths={links[slot].genes,links[slot].convergence};
    if (c.relations==3) p.context=context_tail(links,sites)[slot].genes;
    if (c.programs==2) p.expression=expression_tail(links,sites)[slot].genes;
    if (outcomes) p.outcomes=outcomes[slot].genes;
    if (c.navigation) p.map=map_tail(outcomes,sites)[slot].genes;
    if (c.navigation>=4) p.trace=trace_tail(outcomes,sites)[slot].genes;
}
ENTRY void dispersal_hatch(Site *world,Genome *genomes,SynapticLayer *dendrites,AssociationLayer *links,
                          OutcomeLayer *outcomes,Settings c,const Propagule *pool,Word count,Word seed) {
    Word pos=index(),sites=c.side*c.side;
    if (pos>=sites || !world[pos].body.energy) return;
    const auto &p=pool[mix(pos^seed^0x9e3f4b57u)%count];
    auto &body=world[pos].body;
    body.founder=p.ancestor; body.generation=p.generation;
    genomes[pos]=p.genome;
    if (dendrites) dendrites[pos].genes=p.dendrites;
    if (links) { links[pos].genes=p.paths.genes; links[pos].convergence=p.paths.convergence; }
    if (c.relations==3) context_tail(links,sites)[pos].genes=p.context;
    if (c.programs==2) expression_tail(links,sites)[pos].genes=p.expression;
    if (outcomes) outcomes[pos].genes=p.outcomes;
    if (c.navigation) map_tail(outcomes,sites)[pos].genes=p.map;
    if (c.navigation>=4) trace_tail(outcomes,sites)[pos].genes=p.trace;
}

// Resample only the inherited programs of living parents. Newborn positions,
// age, energy, spike histories and fields come from a freshly initialized world.
ENTRY void modifier_founders(Genome *genes,Word sites,Word seed,Word origin,Word mouths) {
    Word pos=index();
    if (pos>=sites) return;
    if (origin) {
        Word bits=origin==2 ? mix(pos^seed^0x3c87af19u)&0xfff00000u : 0;
        genes[pos].reproduction=(genes[pos].reproduction&0x000fffffu)|bits;
    }
    if (mouths) {
        Word bits=mouths==2 ? mix(pos^seed^0x7918db21u)&0x30000000u : 0;
        genes[pos].body=(genes[pos].body&0xcfffffffu)|bits;
    }
}
ENTRY void plant_transplant(Plant *plants,unsigned char *pods,Settings c,const Plant *stems,const Word *reactions,
                            const Word *materials,Word source_side,Word source_climates) {
    Word pos=index(),sites=c.side*c.side;
    if (pos>=sites) return;
    Word source=(pos%c.side)%source_side+((pos/c.side)%source_side)*source_side, area=source_side*source_side;
    if (stems) {
        plants[pos]=stems[source];
        for (Word phase=1;phase<c.climates;++phase) {
            Word from=source_climates ? phase%source_climates : 0;
            reinterpret_cast<Word *>(plants+sites)[(phase-1)*sites+pos]=from ? reactions[(from-1)*area+source] : stems[source].reaction;
        }
        if (c.fruit==Fruit::inherited) pods[pos]=plant_fruit(stems[source],mix(mix(pos^c.seed)+1),c.species);
    }
    if (materials) food_chemistry(plants,sites,c.climates)[pos]=materials[source];
}
ENTRY void life_transplant(const Word *source, const Genome *ancestors, Word mask, Word compact,
                           Word survivors, Genome *genes, Settings c, const SynapticGenes *donor_layers, SynapticLayer *layers, const HeritableLinks *source_links, AssociationLayer *links, const ContextGenes *contextual_donor,const ExpressionGenes *expression_donor,Graft part,Word pool,Word pools,const OutcomeGenes *food_donors,OutcomeLayer *outcomes,const MapGenes *map_donors,const TraceGenes *trace_donors) {
    Word pos = index();
    if (pos >= c.side * c.side) return;
    bool cross = c.contest == Contest::neural_genes || c.contest == Contest::body_genes;
    if (part==Graft::whole && (c.contest == Contest::ancestry || cross) && !treatment(pos, c)) return;
    bool memory=part==Graft::memory || part==Graft::alternative;
    bool first=part!=Graft::alternative,second=c.programs==2;
    if (memory && pools>1) {
        first=mix(pos^c.seed^0xbc808fb1u)%pools==pool;
        second=second && mix(pos^c.seed^0x5f1d6273u)%pools==pool;
        if (!first && !second) return;
    }
    Word key = mix(pos ^ c.seed ^ 0x3c83d239u), slot;
    if (compact) {
        slot = key % survivors;
    } else {
        Word stride = (mix(key + 1) & mask) | 1, candidate = key & mask;
        while (source[candidate]==~0u) candidate = (candidate + stride) & mask;
        slot = source[candidate];
    }
    if (memory) {
        if (first) context_tail(links,c.side*c.side)[pos].genes=contextual_donor[slot];
        if (second) {
            auto &ex=expression_tail(links,c.side*c.side)[pos].genes;
            if (part==Graft::alternative) ex.alternative=expression_donor ? expression_donor[slot].alternative : contextual_donor[slot];
            else if (expression_donor) ex=expression_donor[slot]; else ex.alternative=contextual_donor[slot];
        }
        return;
    }
    if (c.navigation>=4 && trace_donors && c.contest!=Contest::body_genes) trace_tail(outcomes,c.side*c.side)[pos].genes=trace_donors[slot];
    if (part==Graft::temporal) return;
    if (part==Graft::navigation) { map_tail(outcomes,c.side*c.side)[pos].genes=map_donors[slot]; return; }
    if (part==Graft::outcomes) { outcomes[pos].genes=food_donors[slot]; return; }
    if (part==Graft::pathways) {
        links[pos].genes=source_links[slot].genes;
        links[pos].convergence=source_links[slot].convergence;
        return;
    }
    Genome donor=ancestors[slot];
    Genome &recipient=genes[pos];
    if (part==Graft::fidelity) { recipient.reproduction=(recipient.reproduction&0x000fffffu)|(donor.reproduction&0xfff00000u); return; }
    if (part==Graft::body) {
        recipient.body=donor.body; recipient.reproduction=donor.reproduction;
        recipient.effectors[0]=donor.effectors[0]; recipient.effectors[1]=donor.effectors[1];
        return;
    }
    if (part==Graft::dendrites) {
        recipient.expression=(recipient.expression&65535)|(donor.expression&0xffff0000u);
        if (donor_layers) layers[pos].genes=donor_layers[slot];
        return;
    }
    if (part==Graft::soma) {
        UNROLL
        for (int i=0;i<8;++i) recipient.op[i]=donor.op[i];
        recipient.edges[0]=donor.edges[0]; recipient.edges[1]=donor.edges[1];
        recipient.development=donor.development; recipient.motors=donor.motors; recipient.receptors=donor.receptors;
        recipient.expression=(recipient.expression&0xffff0000u)|(donor.expression&65535);
        return;
    }
    if (cross) {
        Genome fresh = genes[pos];
        Genome peripheral = c.contest == Contest::body_genes ? donor : fresh;
        if (c.contest == Contest::body_genes) donor = fresh;
        donor.body = peripheral.body;
        donor.reproduction = peripheral.reproduction;
        donor.effectors[0] = peripheral.effectors[0]; donor.effectors[1] = peripheral.effectors[1];
    }
    genes[pos] = donor;
    if (c.navigation && map_donors && c.contest!=Contest::body_genes) map_tail(outcomes,c.side*c.side)[pos].genes=map_donors[slot];
    if (outcomes && c.contest!=Contest::body_genes) {
        auto &organ=outcomes[pos].genes;
        if (food_donors) organ=food_donors[slot];
        else {
            Word ans=donor.expression>>16, copy=mix(key^0xbe3d567bu);
            if (copy&1) organ.plastic=(organ.plastic&0xffff0000u)|ans;
            if (copy&2) organ.plastic=(organ.plastic&65535)|(ans<<16);
            if (contextual_donor && (copy&4)) organ.wiring=(organ.wiring&~63u)|((contextual_donor[slot].receptors>>20)&63);
        }
    }
    if (links && source_links && c.contest != Contest::body_genes) {
        links[pos].genes = source_links[slot].genes;
        links[pos].convergence = source_links[slot].convergence;
        if (c.relations==3 && contextual_donor)
            context_tail(links,c.side*c.side)[pos].genes=contextual_donor[slot];
        else if (c.relations==3) {
            auto &newborn=context_tail(links,c.side*c.side)[pos].genes;
            Word duplication=mix(key^0xd482716bu), inherited=donor.expression>>16;
            if (duplication&1) newborn.plastic=(newborn.plastic&0xffff0000u) | (duplication&2 ? inherited : source_links[slot].genes.plastic&65535);
            if (duplication&4) newborn.plastic=(newborn.plastic&65535) | ((duplication&8 ? inherited : source_links[slot].genes.plastic>>16)<<16);
        }
        if (c.programs==2) {
            auto &expression=expression_tail(links,c.side*c.side)[pos].genes;
            if (expression_donor) expression=expression_donor[slot];
            else expression.alternative=context_tail(links,c.side*c.side)[pos].genes;
        }
    }
    if (layers && donor_layers && c.contest != Contest::body_genes)
        layers[pos].genes = donor_layers[slot];
}

ENTRY void life_rules(const Site *world, const Genome *genes, Word sites, Word *counts) {
    Word pos = index();
    if (pos >= sites) return;
    const Body &b = world[pos].body;
    if (!b.energy) return;
    Word rule = genes[b.slot].expression >> 16;
    asm volatile("red.global.add.u32 [%0], 1;" :: "l"(counts + rule) : "memory");
}

#endif
#if defined(AX_FAMILY_assays) || defined(AX_FAMILY_memories) || defined(AX_FAMILY_appraisals) || defined(AX_FAMILY_travel) || defined(AX_FAMILY_mazes) || defined(AX_FAMILY_springbeds) || defined(AX_FAMILY_junctions)
#include "conditioning.cuh"
#endif
#if defined(AX_FAMILY_memories)
#include "reinstatement.cuh"
#endif
#if defined(AX_FAMILY_actors) || defined(AX_FAMILY_foodactors) || defined(AX_FAMILY_mapactors)
#include "actor.cuh"
#endif

#if defined(AX_FAMILY_travel) || defined(AX_FAMILY_mazes) || defined(AX_FAMILY_springbeds) || defined(AX_FAMILY_junctions)
#include "traveller.cuh"
#endif
#if defined(AX_FAMILY_junctions)
#include "junctions.cuh"
#endif
#if defined(AX_FAMILY_travel)
#include "crossings.cuh"
#endif
#if defined(AX_FAMILY_mazes) || defined(AX_FAMILY_springbeds)
#include "terrain.cuh"
#endif
#if defined(AX_FAMILY_mazes)
#include "labyrinth.cuh"
#endif
#if defined(AX_FAMILY_springbeds)
#include "migration.cuh"
#endif
#if defined(AX_FAMILY_appraisals)
#if !defined(AX_SEQUENCE)
#include "valuation.cuh"
#include "subsistence.cuh"
#endif
#include "field_food.cuh"
#endif
#if defined(AX_FAMILY_outcomes) || defined(AX_FAMILY_trails)
#ifdef AX_FAMILY_trails
#define NAVIGATES true
#else
#define NAVIGATES false
#endif
#define FOOD_ARGS const Site *w,const Genome *g,Tissue *b,Intent *a,const Word *clock,Word offset,Settings c,Contacts *d,const unsigned char *pods,DelayLine *h,SynapticLayer *s,AssociationLayer *p,OutcomeLayer *outcomes
#define FOOD_BRAIN(n,l) \
    ENTRY void food##l##_think##n(FOOD_ARGS) { schedule<n,true,false,l,true,NAVIGATES>(w,g,b,a,clock,offset,c,d,pods,h,s,p,outcomes); } \
    ENTRY void food##l##_packed##n(FOOD_ARGS) { schedule<n,true,true,l,true,NAVIGATES>(w,g,b,a,clock,offset,c,d,pods,h,s,p,outcomes); }
#define FOOD_LEVELS(n) FOOD_BRAIN(n,0) FOOD_BRAIN(n,1) FOOD_BRAIN(n,2) FOOD_BRAIN(n,3)
FOOD_LEVELS(2)
FOOD_LEVELS(4)
FOOD_LEVELS(8)
FOOD_LEVELS(16)
FOOD_LEVELS(32)
#undef FOOD_LEVELS
#undef FOOD_BRAIN
#undef FOOD_ARGS
#endif
