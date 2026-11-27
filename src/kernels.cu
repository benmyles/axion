#include "axion/arena.hpp"
#include "axion/planning.hpp"

// Clang's freestanding CUDA frontend and the PTX ISA are the entire device
// toolchain. All rendezvous below are either whole warps or whole CTAs.
#define KERNEL extern "C" __attribute__((global))
#define SHARED __attribute__((shared))
#define UNROLL _Pragma("unroll")
using namespace axion;

namespace {
AX_FN unsigned thread_id() {
    unsigned x;
    asm("mov.u32 %0, %%tid.x;" : "=r"(x));
    return x;
}
AX_FN unsigned block_id() {
    unsigned x;
    asm("mov.u32 %0, %%ctaid.x;" : "=r"(x));
    return x;
}
AX_FN unsigned block_size() {
    unsigned x;
    asm("mov.u32 %0, %%ntid.x;" : "=r"(x));
    return x;
}
AX_FN Word ballot(bool fired) {
    Word bits;
    asm volatile("{ .reg .pred p; setp.ne.u32 p, %1, 0; vote.sync.ballot.b32 %0, p, -1; }"
        : "=r"(bits) : "r"(int(fired)));
    return bits;
}
AX_FN int warp_sum(int value) {
    int sum;
    asm volatile("redux.sync.add.s32 %0, %1, -1;" : "=r"(sum) : "r"(value));
    return sum;
}
AX_FN void barrier() { asm volatile("bar.sync 0;" ::: "memory"); }

struct DeviceTotals { unsigned long long wins, ticks, spikes, matches; Word image, padding; };
static_assert(sizeof(DeviceTotals) == 40);

template<int Tiles, Learning L>
struct WarpNetwork {
    Cell<L> cells[Tiles];
    Word spikes[Tiles];
    int lane;

    AX_FN void reset(Word key) {
        UNROLL
        for (int g = 0; g < Tiles; ++g) {
            spikes[g] = 0;
            cells[g].reset(key, 32 * g + lane);
        }
    }

    AX_FN void tick(Word input) {
        Word next[Tiles];
        int sensors = population(input);
        UNROLL
        for (int g = 0; g < Tiles; ++g) {
            Word remote = spikes[g ^ 1];
            if constexpr (Tiles == 4) {
                // Select values, never addresses. A dynamic subscript forces the
                // compiler to materialize the tile array in thread-local memory.
                asm("{ .reg .pred p; setp.ne.u32 p, %3, 0; selp.b32 %0, %1, %2, p; }"
                    : "=r"(remote) : "r"(spikes[g ^ 3]), "r"(remote), "r"(lane & 1));
            }
            Word local = spikes[g];
            bool fired = cells[g].tick(32 * g + lane, input, local, remote,
                                        active_cost(sensors, local, remote));
            next[g] = ballot(fired);
        }
        UNROLL
        for (int g = 0; g < Tiles; ++g) spikes[g] = next[g];
    }

    AX_FN SpikeCode signature(Word receptors) const {
        int active = population(receptors);
        return {receptors, ballot(onset_spike(cells[Tiles/2].weights.local, receptors, active))};
    }

    AX_FN Sums read() const {
        Sums sum{};
        UNROLL
        for (int g = 0; g < Tiles; ++g) {
            auto x = cells[g].read();
            sum.left += x.left;
            sum.right += x.right;
            sum.mass += x.mass;
        }
        return {warp_sum(sum.left), warp_sum(sum.right), warp_sum(sum.mass)};
    }

    AX_FN void commit(int action, int q) {
        UNROLL
        for (int g = 0; g < Tiles; ++g) cells[g].commit(action, q);
    }
    AX_FN void reinforce(int error) {
        UNROLL
        for (int g = 0; g < Tiles; ++g) cells[g].reinforce(error);
    }
    AX_FN Word spike_count() const {
        int count = 0;
        UNROLL
        for (int g = 0; g < Tiles; ++g) count += int(cells[g].spikes);
        return unsigned(warp_sum(count));
    }
};

template<int TileCapacity>
struct Exchange {
    Word spikes[2][TileCapacity];
    Sums partial[32];
};

// Large stripes keep infrequently used motor weights and eligibility in
// bank-aligned shared planes across ticks. This leaves registers for recurrence.
template<int Tiles, bool Stage>
struct BlockStorage : Exchange<Tiles> {};
template<int Tiles>
struct BlockStorage<Tiles, true> : Exchange<Tiles> {
    short motor[Tiles * 32][2];
    Word tags[Tiles * 32];
};

template<int Stripes, int TileCapacity, Learning L>
struct BlockNetwork {
    Cell<L> cells[Stripes];
    static constexpr bool staged = Stripes == 8 && L != Learning::frozen;
    BlockStorage<TileCapacity, staged> *shared;
    int id, width, warps, tiles, lane, phase;
    Word stripe_spikes;
    Word hot[Stripes];

    AX_FN void reset(Word key) {
        phase = 0;
        UNROLL
        for (int k = 0; k < Stripes; ++k) {
            int index = id + k * width;
            cells[k].reset(key, index);
            if constexpr (staged) {
                shared->tags[index] = 1u << 20;
                hot[k] = Activity{}.pack(0);
            }
            if (lane == 0) shared->spikes[0][index / 32] = 0;
        }
        barrier();
    }

    AX_FN void tick(Word input) {
        int sensors = population(input);
        UNROLL
        for (int k = 0; k < Stripes; ++k) {
            int index = id + k * width;
            int tile = index / 32;
            Word local = shared->spikes[phase][tile];
            Word far = shared->spikes[phase][remote_tile(tile, index, tiles)];
            if constexpr (staged) {
                Word h = hot[k];
                cells[k].state = Activity::unpack(h);
                cells[k].tag.age = int(h >> 24);
            }
            bool fired = cells[k].tick(index, input, local, far, active_cost(sensors, local, far));
            if constexpr (staged) hot[k] = cells[k].state.pack(cells[k].tag.age);
            if constexpr (Stripes == 8) stripe_spikes += unsigned(fired);
            Word bits = ballot(fired);
            if (lane == 0) shared->spikes[phase ^ 1][tile] = bits;
        }
        phase ^= 1;
        barrier();
    }

    AX_FN void load_motor(int k) {
        if constexpr (staged) {
            int i = id + k * width;
            cells[k].weights.motor[0] = shared->motor[i][0];
            cells[k].weights.motor[1] = shared->motor[i][1];
        }
    }

    AX_FN void store_motor(int k) {
        if constexpr (staged) {
            int i = id + k * width;
            shared->motor[i][0] = cells[k].weights.motor[0];
            shared->motor[i][1] = cells[k].weights.motor[1];
        }
    }

    AX_FN void load_decision(int k) {
        if constexpr (staged) {
            load_motor(k);
            cells[k].tag.unpack_decision(shared->tags[id + k * width]);
        }
    }

    AX_FN SpikeCode signature(Word receptors) {
        int active = population(receptors);
        UNROLL
        for (int k = 0; k < Stripes; ++k) {
            int tile = (id + k * width) / 32;
            if (tile == tiles/2) {
                Word bits = ballot(onset_spike(cells[k].weights.local, receptors, active));
                if (lane == 0) shared->spikes[phase ^ 1][tile] = bits;
            }
        }
        barrier();
        SpikeCode code{receptors, shared->spikes[phase ^ 1][tiles/2]};
        barrier(); // All warps must finish loading before the next tick reuses this buffer.
        return code;
    }

    AX_FN Sums read() {
        Sums sum{};
        UNROLL
        for (int k = 0; k < Stripes; ++k) {
            if constexpr (staged) cells[k].state.trace = int((hot[k] >> 16) & 255);
            load_motor(k);
            auto x = cells[k].read();
            sum.left += x.left;
            sum.right += x.right;
            sum.mass += x.mass;
        }
        Sums local{warp_sum(sum.left), warp_sum(sum.right), warp_sum(sum.mass)};
        if (lane == 0) shared->partial[id / 32] = local;
        barrier();
        auto y = lane < warps ? shared->partial[lane] : Sums{};
        return {warp_sum(y.left), warp_sum(y.right), warp_sum(y.mass)};
    }

    AX_FN void commit(int action, int q) {
        UNROLL
        for (int k = 0; k < Stripes; ++k) {
            if constexpr (staged) {
                cells[k].state.trace = int((hot[k] >> 16) & 255);
                cells[k].tag.age = int(hot[k] >> 24);
            }
            load_decision(k);
            cells[k].commit(action, q);
            if constexpr (staged) shared->tags[id + k * width] = cells[k].tag.pack_decision();
        }
    }
    AX_FN void reinforce(int error) {
        UNROLL
        for (int k = 0; k < Stripes; ++k) {
            load_decision(k);
            cells[k].reinforce(error);
            store_motor(k);
        }
    }
    AX_FN Word spike_count() const {
        int count = 0;
        if constexpr (Stripes == 8) count = int(stripe_spikes);
        else {
            UNROLL
            for (int k = 0; k < Stripes; ++k) count += int(cells[k].spikes);
        }
        count = warp_sum(count);
        if (lane == 0) shared->partial[id / 32].mass = count;
        barrier();
        return unsigned(warp_sum(lane < warps ? shared->partial[lane].mass : 0));
    }
};

struct DistributedMemory {
    Engram row;
    RecallState state;
    Archive archive;
    bool writer;
    int lane;

    AX_FN Votes read(const Engram &q, Recall mode) const {
        Votes v;
        if (!state.used && !archive.entries) return v;
        if (Word(lane) < state.used) v = recall_row(q, row, mode);
        v.base = warp_sum(v.base);
        UNROLL
        for (int h = 0; h < channels; ++h) {
            v.preference[h] = warp_sum(v.preference[h]);
            v.mass[h] = warp_sum(v.mass[h]);
        }
        if (archive.entries) archive.vote(v, q);
        return v;
    }
    AX_FN SpikeCode translate(SpikeCode cue) const {
        if (!state.used && !archive.entries) return {};
        int score = Word(lane) < state.used ? binding_score(cue, row, lane) : -1;
        int best;
        asm volatile("redux.sync.max.s32 %0, %1, -1;" : "=r"(best) : "r"(score));
        if (Word durable = archive.recall(cue.lo, best >> 5)) return {durable, 0};
        if (best < 32 * 56) return {};
        int slot = 31 - (best & 31);
        SpikeCode remembered;
        asm volatile("shfl.sync.idx.b32 %0, %1, %2, 31, -1;"
                     : "=r"(remembered.lo) : "r"(row.item[3].lo), "r"(slot));
        asm volatile("shfl.sync.idx.b32 %0, %1, %2, 31, -1;"
                     : "=r"(remembered.hi) : "r"(row.item[3].hi), "r"(slot));
        return remembered;
    }
    AX_FN void write(const Engram &encoded) {
        if (Word(lane) == state.next) {
            if (writer && archive.entries && state.used == memory_rows) archive.consolidate(row);
            row = encoded;
        }
        if (archive.entries) asm volatile("bar.warp.sync -1;" ::: "memory");
    }
};

// One warp owns a register-resident copy of the place bank. A wave expands
// every learned predecessor in one ballot, irrespective of its place index.
struct DistributedPlaces {
    Place place;
    Word used, lane;

    AX_FN int identify(SpikeCode code, bool learn) {
        if (!(code.lo | code.hi)) return -1;
        int score = lane < used ? best_match(code, place, lane) : -1;
        int best;
        asm volatile("redux.sync.max.s32 %0, %1, -1;" : "=r"(best) : "r"(score));
        int slot = match_slot(best, learn);
        if (slot < 0 && learn && used < 32) {
            slot = int(used++);
            if (lane == Word(slot)) place = {code, {}};
        }
        return slot;
    }
    AX_FN Doors doors(int slot) const {
        Doors result;
        for (int i = 0; i < 2; ++i)
            asm volatile("shfl.sync.idx.b32 %0, %1, %2, 31, -1;"
                         : "=r"(result.arm[i]) : "r"(place.exits.arm[i]), "r"(slot));
        return result;
    }
    AX_FN void link(int from, int action, int to) {
        if (lane == Word(from)) {
            if (action) place.exits.arm[1] = 1u << to;
            else place.exits.arm[0] = 1u << to;
        }
    }
    AX_FN Word frontier() const {
        return ballot(lane < used && (!place.exits.arm[0] || !place.exits.arm[1]));
    }
    AX_FN Word predecessors(Word active) const {
        return ballot(lane < used && (active & (place.exits.arm[0] | place.exits.arm[1])));
    }
};

AX_FN Word shuffle(Word value, int slot) {
    Word x;
    asm volatile("shfl.sync.idx.b32 %0, %1, %2, 31, -1;" : "=r"(x) : "r"(value), "r"(slot));
    return x;
}

struct WarpSearch {
    const InterventionMemory *experience;
    Word slots{};
    Word first_four{}, last_four{}, length;
    int lane;

    AX_FN void put(Word step, Word literal) {
        Word shifted = literal << (8 * (step & 3));
        if (step < 4) first_four |= shifted;
        else last_four |= shifted;
    }
    AX_FN int score(Word input, Word target, bool perturb, int salt) const {
        if (experience) {
            Word image = input ^ (perturb ? 1u << lane : 0u);
            for (Word k = 0; k < length; ++k) {
                Word literal = ((k < 4 ? first_four : last_four) >> (8 * (k & 3))) & 255;
                const auto &table = experience->circuit[(slots >> (2 * k)) & 3];
                Word next = 0;
                Inputs x(image);
                for (int j = 0; j < 32; ++j) {
                    Word f = shuffle(literal, j);
                    int bit = table.predict(image, j);
                    if (bit < 0) bit = ((x.branch[f >> 6] >> (f & 31)) ^ (f >> 5)) & 1;
                    next |= Word(bit) << j;
                }
                image = next;
            }
            int best, cost = 32 * population(image ^ target) + (lane ^ salt);
            asm volatile("redux.sync.min.s32 %0, %1, -1;" : "=r"(best) : "r"(cost));
            return best;
        }
        Word plane = (0u - ((input >> lane) & 1)) ^ (perturb ? 1u << lane : 0u);
        for (Word k = 0; k < length; ++k) {
            Word f = ((k < 4 ? first_four : last_four) >> (8 * (k & 3))) & 255;
            Word source = shuffle(plane, f & 31);
            Word neighbor = shuffle(plane, neighbor_of(f));
            plane = apply_literal(f, source, neighbor);
        }
        UNROLL
        for (int d = 1; d < 32; d *= 2) {
            Word other = shuffle(plane, lane ^ d);
            Word keep = ~0u / ((1u << d) + 1);
            plane = (lane & d) ? (plane & ~keep) | ((other & ~keep) >> d)
                               : (plane & keep) | ((other & keep) << d);
        }
        int cost = 32 * population(plane ^ target) + (lane ^ salt), best;
        asm volatile("redux.sync.min.s32 %0, %1, -1;" : "=r"(best) : "r"(cost));
        return best;
    }
};

struct WarpConstraints {
    LearnedGate gates[4];
    Word slots{};
    unsigned short choices[8]{}, levels[9]{};
    int lane;

    AX_FN Implications filter(int stage, Clamps in, Clamps out) const {
        auto r = gates[(slots >> (2 * stage)) & 3].filter(in, out, lane, stage);
        asm volatile("redux.sync.or.b32 %0, %1, -1;" : "=r"(r.input.on) : "r"(r.input.on));
        asm volatile("redux.sync.or.b32 %0, %1, -1;" : "=r"(r.input.off) : "r"(r.input.off));
        r.output.on = ballot(bool(r.output.on));
        r.output.off = ballot(bool(r.output.off));
        asm volatile("redux.sync.min.u32 %0, %1, -1;" : "=r"(r.choice) : "r"(r.choice));
        return r;
    }
    AX_FN void stamp(int stage, Word bits, int depth) {
        if ((bits >> lane) & 1) levels[stage] = static_cast<unsigned short>(depth);
    }
    AX_FN Word retract(int stage, int depth) {
        bool erase = levels[stage] >= depth;
        if (erase) levels[stage] = 0;
        return ballot(erase);
    }
    AX_FN void save(int depth, Word choice) {
        if (lane == (depth & 31)) choices[depth / 32] = static_cast<unsigned short>(choice);
    }
    AX_FN Word saved(int depth) const { return shuffle(choices[depth / 32], depth & 31); }
};

struct WarpPredictor {
    struct Program {
        Word token{};
        AX_FN void put(Word step, Word cue) { if ((thread_id() & 31) == step) token = cue; }
        AX_FN Word get(Word step) const { return shuffle(token, step); }
    };
    DrawingMemory *memory;
    InterventionMemory *experience;
    Palette palette;
    int lane;

    AX_FN Word previous(Word cue, Word fallback) const {
        int slot = experience && memory ? palette.find(cue) : -1;
        if (slot < 0 || !experience->circuit[slot].observed) return fallback;
        return experience->circuit[slot].previous_input;
    }
    AX_FN int uncertainty(Word cue, Word a, Word b) const {
        int slot = experience && memory ? palette.find(cue) : -1;
        if (slot < 0) return 0;
        const auto &table = experience->circuit[slot];
        return warp_sum(table.unknown(a, lane) + table.unknown(b, lane));
    }
    AX_FN Transition residual(const Span &span, Word image, Word result) const {
        Word active = image ^ span.anchor;
        auto row = span.row[lane];
        Word input = (active >> lane) & 1 ? row.input : 0;
        Word output = (active >> lane) & 1 ? row.output : 0;
        asm volatile("redux.sync.xor.b32 %0, %1, -1;" : "=r"(input) : "r"(input));
        asm volatile("redux.sync.xor.b32 %0, %1, -1;" : "=r"(output) : "r"(output));
        return {active ^ input, result ^ span.response ^ output};
    }
    AX_FN Word apply(Word cue, Word picture, Recall mode) const {
        int slot = palette.find(cue);
        if (slot < 0 || mode == Recall::cortical) return picture;
        const Span &span = memory->spans[slot];
        if (mode == Recall::associative && span.confirmed >= 2) {
            auto delta = residual(span, picture, 0);
            if (!delta.input) return delta.output;
        }
        int value = experience && mode == Recall::associative ? experience->circuit[slot].predict(picture, lane) : -1;
        if (value < 0) value = memory->circuit[slot].fire(Inputs(picture), lane, mode == Recall::content ? 1 : branches);
        return ballot(value);
    }
    AX_FN bool plan(Word target, const Program &program, Word length, Word nodes, Word &answer) const {
        WarpConstraints search;
        search.lane = lane;
        Word loaded = 0;
        for (Word k = 0; k < length; ++k) {
            int slot = palette.find(program.get(k));
            if (slot < 0) return false;
            search.slots |= Word(slot) << (2 * k);
            if ((loaded >> slot) & 1) continue;
            if (ballot(search.gates[slot].read(*memory, experience, slot, lane)) != ~0u) return false;
            loaded |= 1u << slot;
        }
        return solve_program(search, target, length, nodes, answer);
    }
    AX_FN Word imagine(Word target, const Program &program, Word length, Word nodes) const {
        Word answer;
        if (nodes && plan(target, program, length, nodes, answer)) return answer;
        WarpSearch search;
        search.experience = experience;
        search.lane = lane;
        search.length = length;
        for (Word k = 0; k < length; ++k) {
            int slot = palette.find(program.get(k));
            if (slot < 0) return target;
            search.slots |= Word(slot) << (2 * k);
            search.put(k, memory->circuit[slot].hypothesis(lane));
        }
        return improve_input(search, target);
    }
    AX_FN Word construct(Word target, const Program &program, Word length, Recall mode, Word nodes) const {
        if (!memory || mode != Recall::associative) return target;
        Word own = 1u << lane, column = own, source = own, bias = 0;
        for (Word k = 0; k < length; ++k) {
            int slot = palette.find(program.get(k));
            if (slot < 0) return target;
            const auto &span = memory->spans[slot];
            if (span.confirmed < 2) return imagine(target, program, length, nodes);
            auto row = span.row[lane];
            if (ballot(row.input == own) != ~0u) return imagine(target, program, length, nodes);
            bias = residual(span, bias, 0).output;
            Word next = 0;
            for (int j = 0; j < 32; ++j) {
                Word value = shuffle(row.output, j);
                if (column & (1u << j)) next ^= value;
            }
            column = next;
        }
        Word remaining = target ^ bias, answer = 0;
        bool free = true;
        for (int bit = 0; bit < 32 && remaining; ++bit) {
            Word mask = 1u << bit;
            Word eligible = ballot(free && (column & mask));
            if (!eligible) continue;
            int pivot = __builtin_ctz(eligible);
            Word image = shuffle(column, pivot), input = shuffle(source, pivot);
            if (remaining & mask) { remaining ^= image; answer ^= input; }
            if (lane == pivot) free = false;
            if (free && (column & mask)) { column ^= image; source ^= input; }
        }
        return remaining ? target : answer;
    }
    AX_FN void observe(Word cue, Word picture, Word target, Recall mode) {
        int slot = palette.find(cue);
        bool fresh = slot < 0;
        if (fresh) slot = palette.recruit(cue);
        if (experience) {
            auto &table = experience->circuit[slot];
            Word intervention = table.intervention(picture, fresh);
            Word cause = ((target ^ table.previous_output) & (1u << lane)) ? intervention : 0;
            table.witness(lane, picture, (target >> lane) & 1, cause, fresh);
            asm volatile("bar.warp.sync -1;" ::: "memory");
            if (!lane) {
                table.previous_input = picture;
                table.previous_output = target;
                table.observed = 1;
            }
        }
        auto &span = memory->spans[slot];
        bool reset = fresh;
        if (!fresh && mode == Recall::associative) {
            auto delta = residual(span, picture, target);
            if (delta.input) update_pivot(span.row[lane], lane, __builtin_ctz(delta.input), delta);
            else if (delta.output) reset = true;
            else if (lane == 0 && span.confirmed < 2) ++span.confirmed;
        }
        if (reset) {
            span.row[lane] = {};
            if (lane == 0) {
                span.anchor = picture;
                span.response = target;
                span.confirmed = 0;
            }
        }
        memory->circuit[slot].observe(Inputs(picture), lane, (target >> lane) & 1,
                                     mode == Recall::content ? 1 : branches, fresh);
        asm volatile("bar.warp.sync -1;" ::: "memory");
    }
};

enum class Family { foraging, lessons, routes };
template<Family F, Learning L, class Network>
AX_FN Counts interact(Network &net, MemoryBank *banks, PlaceMemory *maps, DrawingMemory *drawing, Association *archive, Course course,
                      Word animal, Word first, Word count, bool owner_warp) {
    if constexpr (F == Family::foraging) return simulate<L>(net, course, animal, first, count);
    DistributedMemory memory{};
    memory.lane = int(thread_id() & 31);
    memory.writer = owner_warp;
    memory.archive = {archive ? archive + animal * course.archive_slots : nullptr, course.archive_slots};
    if (banks) {
        memory.state.load(banks[animal]);
        memory.row = banks[animal].rows[memory.lane];
    }
    Counts out;
    if constexpr (F == Family::routes) {
        DistributedPlaces places{};
        places.lane = thread_id() & 31;
        if (maps) {
            places.place = maps[animal].places[places.lane];
            places.used = maps[animal].used;
        }
        out = navigate<L>(net, places, memory, course, animal, first, count);
        if constexpr (L != Learning::frozen) {
            if (owner_warp && maps) {
                maps[animal].places[places.lane] = places.place;
                if (places.lane == 0) maps[animal].used = places.used;
            }
        }
    } else if (course.task == Task::detector) {
        out = detector_trials<L>(net, memory, course, animal, first, count);
    } else if (course.recall == Recall::cortical) {
        out = study_cortex<L>(net, course, animal, first, count);
    } else out = study<L>(net, memory, course, animal, first, count);
    if constexpr (L != Learning::frozen) {
        if (owner_warp && banks) {
            banks[animal].rows[memory.lane] = memory.row;
            if (memory.lane == 0) memory.state.save(banks[animal]);
        }
    }
    return out;
}

// Phase boundaries live entirely inside this launch. The host sees weights and
// aggregate counters only between batches of complete episodes.
#define PARAMS Weights *weights, DeviceTotals *stats, Course course, Word animals, Word first, Word count, MemoryBank *banks, PlaceMemory *maps, DrawingMemory *drawing, Association *archive, InterventionMemory *interventions
#define ARGS weights, stats, course, animals, first, count, banks, maps, drawing, archive, interventions

AX_FN void record(DeviceTotals &total, Counts part) {
    total.wins += part.wins;
    total.ticks += part.ticks;
    total.spikes += part.spikes;
    total.matches += part.matches;
    total.image = part.image;
}

// Each lane owns one generative output neuron. A picture update never
// launches the foraging circuit, even when that circuit has a million cells.
template<Learning L, bool Investigate, bool ForwardOnly>
AX_FN void run_drawing(PARAMS) {
    (void)weights; (void)banks; (void)maps;
    Word animal = block_id() * (block_size() / 32) + thread_id() / 32;
    if (animal >= animals) return;
    if constexpr (!Investigate) course.experiments = Course::Experiments::off;
    auto *experimental = Investigate && interventions ? interventions + animal : nullptr;
    WarpPredictor predictor{drawing ? drawing + animal : nullptr, experimental, {}, int(thread_id() & 31)};
    if (drawing) predictor.palette = drawing[animal].palette;
    auto result = draw_trials<L, ForwardOnly>(predictor, course, animal, first, count);
    if (predictor.lane == 0) {
        if constexpr (L != Learning::frozen) if (drawing) drawing[animal].palette = predictor.palette;
        record(stats[animal], result);
    }
}

template<int Tiles, Learning L, Family F>
AX_FN void run_warp(PARAMS) {
    int thread = int(thread_id());
    Word animal = block_id() * (block_size() / 32) + unsigned(thread / 32);
    if (animal >= animals) return;
    Weights *base = weights + animal * (32 * Tiles);
    WarpNetwork<Tiles, L> net;
    net.lane = thread & 31;
    UNROLL
    for (int g = 0; g < Tiles; ++g) {
        net.cells[g].weights = base[32 * g + net.lane];
        net.cells[g].spikes = 0;
    }
    auto result = interact<F, L>(net, banks, maps, drawing, archive, course, animal, first, count, true);
    if constexpr (L != Learning::frozen) {
        UNROLL
        for (int g = 0; g < Tiles; ++g) base[32 * g + net.lane] = net.cells[g].weights;
    }
    if (net.lane == 0) record(stats[animal], result);
}

template<int Stripes, int TileCapacity, Learning L, Family F>
AX_FN void run_block(PARAMS) {
    Word animal = block_id();
    if (animal >= animals) return;
    int id = int(thread_id());
    int width = int(block_size());
    SHARED BlockStorage<TileCapacity, BlockNetwork<Stripes, TileCapacity, L>::staged> exchange;
    BlockNetwork<Stripes, TileCapacity, L> net;
    net.shared = &exchange;
    net.id = id;
    net.width = width;
    net.warps = width / 32;
    net.stripe_spikes = 0;
    net.tiles = net.warps * Stripes;
    net.lane = id & 31;
    auto *base = weights + animal * width * Stripes;
    UNROLL
    for (int k = 0; k < Stripes; ++k) {
        net.cells[k].weights = base[k * width + id];
        net.cells[k].spikes = 0;
        net.store_motor(k);
    }
    auto result = interact<F, L>(net, banks, maps, drawing, archive, course, animal, first, count, id < 32);
    if constexpr (L != Learning::frozen) {
        UNROLL
        for (int k = 0; k < Stripes; ++k) {
            net.load_motor(k);
            base[k * width + id] = net.cells[k].weights;
        }
    }
    if (id == 0) record(stats[animal], result);
}
} // namespace

#define ENTRY_FAMILY(prefix, mode, school) \
    KERNEL void prefix##warp64_##mode(PARAMS) { run_warp<2, Learning::mode, school>(ARGS); } \
    KERNEL void prefix##warp128_##mode(PARAMS) { run_warp<4, Learning::mode, school>(ARGS); } \
    KERNEL __attribute__((launch_bounds(1024))) void prefix##block_##mode(PARAMS) { run_block<1, 32, Learning::mode, school>(ARGS); } \
    KERNEL __attribute__((launch_bounds(512))) void prefix##striped2048_##mode(PARAMS) { run_block<4, 64, Learning::mode, school>(ARGS); } \
    KERNEL __attribute__((launch_bounds(512))) void prefix##striped4096_##mode(PARAMS) { run_block<8, 128, Learning::mode, school>(ARGS); }

#if defined(AX_FAMILY_foraging)
#define ENTRY_POINTS(mode) ENTRY_FAMILY(, mode, Family::foraging)
#define GRID_ENTRY(mode) GRID_FAMILY(, mode, Family::foraging)
#elif defined(AX_FAMILY_lessons)
#define ENTRY_POINTS(mode) ENTRY_FAMILY(lesson_, mode, Family::lessons)
#define GRID_ENTRY(mode) GRID_FAMILY(lesson_, mode, Family::lessons)
#elif defined(AX_FAMILY_routes)
#define ENTRY_POINTS(mode) ENTRY_FAMILY(route_, mode, Family::routes)
#define GRID_ENTRY(mode) GRID_FAMILY(route_, mode, Family::routes)
#elif defined(AX_FAMILY_drawing)
#define ENTRY_POINTS(mode) \
    KERNEL void draw_##mode(PARAMS) { run_drawing<Learning::mode, false, true>(ARGS); } \
    KERNEL void investigate_##mode(PARAMS) { run_drawing<Learning::mode, true, true>(ARGS); } \
    KERNEL void inverse_##mode(PARAMS) { run_drawing<Learning::mode, false, false>(ARGS); } \
    KERNEL void causal_inverse_##mode(PARAMS) { run_drawing<Learning::mode, true, false>(ARGS); }
#define GRID_ENTRY(mode)
#else
#error Select a CUDA module family
#endif

ENTRY_POINTS(frozen)
ENTRY_POINTS(motor)
ENTRY_POINTS(full)
ENTRY_POINTS(no_scaling)

#include "grid.cuh"
