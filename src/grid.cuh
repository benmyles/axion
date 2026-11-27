// Included inside kernels.cu. The cooperative grid owns one animal. All blocks
// stay resident and exchange a single bit per spike at each recurrent boundary.
namespace {

AX_FN Word grid_size() {
    Word x;
    asm("mov.u32 %0, %%nctaid.x;" : "=r"(x));
    return x;
}

struct GridBarrier {
    Word *tickets, *epoch;
    Word generation;

    AX_FN void sync() {
        ++generation;
        barrier();
        if (thread_id() == 0) {
            Word ticket;
            // The RMW release sequence publishes all CTA writes to the last
            // arrival. That CTA releases the new epoch to every other leader.
            asm volatile("atom.acq_rel.gpu.global.add.u32 %0, [%1], 1;"
                         : "=r"(ticket) : "l"(tickets) : "memory");
            if (ticket + 1 == grid_size()) {
                asm volatile("st.relaxed.gpu.global.u32 [%0], 0;" :: "l"(tickets) : "memory");
                asm volatile("st.release.gpu.global.u32 [%0], %1;" :: "l"(epoch), "r"(generation) : "memory");
            } else {
                Word seen;
                do {
                    asm volatile("ld.acquire.gpu.global.u32 %0, [%1];"
                                 : "=r"(seen) : "l"(epoch) : "memory");
                } while (seen != generation);
            }
        }
        barrier();
    }
};

// The high half fits in s32 even after summing every cell at maximum width.
// Decomposing at bit 16 lets two integer REDUX operations sum signed 64-bit
// readouts without a five-stage shuffle tree.
AX_FN long long wide_warp_sum(long long x) {
    return 65536ll * warp_sum(int(x >> 16)) + warp_sum(int(x & 65535));
}
AX_FN WideSums reduce_warp(WideSums x) {
    return {wide_warp_sum(x.left), wide_warp_sum(x.right), warp_sum(x.mass)};
}

template<Learning L>
struct GridNetwork {
    Weights *weights;
    Word *activity, *spike_words, *eligibility;
    WideSums *partials, *shared;
    GridBarrier gate;
    Word width, index, stride, tiles, phase, key, spike_total;
    bool first_tick;

    AX_FN void reset(Word trial) {
        key = trial;
        first_tick = true;
        phase = 0;
    }

    AX_FN void tick(Word input) {
        int sensors = population(input);
        Word *prev = spike_words + tiles * phase;
        Word *next = spike_words + tiles * (phase ^ 1);
        for (Word i = index; i < width; i += stride) {
            int tile = int(i / 32);
            Word local = first_tick ? 0 : prev[tile];
            Word far = first_tick ? 0 : prev[remote_tile(tile, int(i), int(tiles))];
            Word s = first_tick ? Activity{}.pack(0) : activity[i];
            Cell<L> c{};
            c.weights = weights[i];
            c.state = Activity::unpack(s);
            if constexpr (L != Learning::frozen) {
                if (first_tick) eligibility[i] = 1u << 20;
                if constexpr (plastic_core<L>) {
                    c.tag.sample = probe_sample(key, int(i));
                    c.tag.age = int(s >> 24);
                }
            }
            bool fired = c.tick(int(i), input, local, far, active_cost(sensors, local, far));
            spike_total += unsigned(fired);
            activity[i] = c.state.pack(c.tag.age);
            Word bits = ballot(fired);
            if ((i & 31) == 0) next[tile] = bits;
        }
        phase ^= 1;
        first_tick = false;
        gate.sync();
    }

    AX_FN WideSums block_sum(WideSums x) {
        Word lane = thread_id() & 31;
        auto sum = reduce_warp(x);
        if (lane == 0) shared[thread_id() / 32] = sum;
        barrier();
        return reduce_warp(lane < block_size() / 32 ? shared[lane] : WideSums{});
    }

    AX_FN WideSums sum_grid(WideSums x) {
        auto partial = block_sum(x);
        if (thread_id() == 0) partials[block_id()] = partial;
        gate.sync();
        WideSums sum{};
        for (Word i = thread_id(); i < grid_size(); i += block_size()) {
            auto p = partials[i];
            sum.left += p.left;
            sum.right += p.right;
            sum.mass += p.mass;
        }
        return block_sum(sum);
    }

    AX_FN SpikeCode signature(Word receptors) const {
        Word lane = thread_id() & 31;
        int active = population(receptors);
        return {receptors, ballot(onset_spike(weights[width/2 + lane].local, receptors, active))};
    }

    AX_FN WideSums read() {
        WideSums sum{};
        for (Word i = index; i < width; i += stride) {
            int x = int((activity[i] >> 16) & 255);
            sum.left += weights[i].motor[0] * x;
            sum.right += weights[i].motor[1] * x;
            sum.mass += x;
        }
        return sum_grid(sum);
    }

    AX_FN Cell<L> learning_state(Word i) {
        Cell<L> c{};
        c.weights = weights[i];
        c.tag.unpack_decision(eligibility[i]);
        if constexpr (plastic_core<L>) c.tag.sample = probe_sample(key, int(i));
        return c;
    }
    AX_FN void commit(int action, int q) {
        for (Word i = index; i < width; i += stride) {
            auto c = learning_state(i);
            Word s = activity[i];
            c.state.trace = int((s >> 16) & 255);
            c.tag.age = int(s >> 24);
            c.commit(action, q);
            eligibility[i] = c.tag.pack_decision();
        }
    }
    AX_FN void reinforce(int error) {
        for (Word i = index; i < width; i += stride) {
            auto c = learning_state(i);
            c.reinforce(error);
            weights[i] = c.weights;
        }
    }
    AX_FN Word spike_count() { return Word(sum_grid({0, 0, int(spike_total)}).mass); }
};

#define GRID_PARAMS PARAMS, Word width, Word animal, Word *scratch, Word *eligibility
#define GRID_ARGS ARGS, width, animal, scratch, eligibility

template<Learning L, Family F>
AX_FN void run_grid(GRID_PARAMS) {
    (void)animals;
    constexpr int warp_count = F == Family::foraging && L != Learning::frozen ? 8 : 4;
    SHARED WideSums shared[warp_count];
    auto *spikes = scratch + width;
    auto *tickets = spikes + 2 * (width / 32);
    Word epoch;
    asm volatile("ld.acquire.gpu.global.u32 %0, [%1];" : "=r"(epoch) : "l"(tickets + 1) : "memory");
    GridNetwork<L> net{};
    net.weights = weights + animal * width;
    net.activity = scratch;
    net.spike_words = spikes;
    net.eligibility = eligibility;
    net.partials = reinterpret_cast<WideSums *>(tickets + 2);
    net.shared = shared;
    net.gate = {tickets, tickets + 1, epoch};
    net.width = width;
    net.index = block_id() * block_size() + thread_id();
    net.stride = grid_size() * block_size();
    net.tiles = width / 32;
    auto result = interact<F, L>(net, banks, maps, drawing, archive, course, animal, first, count, net.index < 32);
    if (net.index == 0) record(stats[animal], result);
}
} // namespace

#define GRID_FAMILY(prefix, mode, school) \
    KERNEL __attribute__((launch_bounds(school == Family::foraging && Learning::mode != Learning::frozen ? 256 : 128))) void prefix##grid_##mode(GRID_PARAMS) { run_grid<Learning::mode, school>(GRID_ARGS); }
GRID_ENTRY(frozen)
GRID_ENTRY(motor)
GRID_ENTRY(full)
GRID_ENTRY(no_scaling)
