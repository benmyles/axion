#include "somatic_core.cuh"
AX_FN Word delayed(Word cue, Word motor) {
    return 256 | (motor << 5) | (cue & 31) | ((cue & ~31u) << 4);
}
AX_FN Word conduction(const Genome &g) { return 1 + ((g.edges[0] >> 10) & 7); }

AX_FN Word developmental_contacts(const Genome &g, Word pool) {
    if (g.expression & (1u << 13)) return mix(g.development + 71 * pool);
    return 0u - ((g.expression >> 12) & 1);
}
AX_FN Word receptor(const Genome &g, Word signature) {
    return pattern(signature, (g.edges[1] >> 10) & 31);
}

template<bool Grove>
AX_FN Word select_motor(const Genome &g,MotorCounts counts,Word tie,Word dendritic=0) {
    Word best=0, action=0;
    UNROLL
    for (Word a=0;a<8;++a) {
        Word votes=((a<4 ? counts.low : counts.high)>>(8*(a&3)))&255;
        if constexpr (Grove) votes+=((dendritic>>(4*(a^(g.motors>>29))))&15)*counts.gain;
        Word score=votes*8+((tie^a)&7);
        if (!a || score>best) { best=score; action=a; }
    }
    return action^(g.motors>>29);
}

#include "associations.cuh"
#include "assemblies.cuh"

template<int Tiles, int Relational = 0>
struct Dendrites : Paths<Tiles,Relational>, Assemblies<Tiles,Relational == 3> {
    using Path = Paths<Tiles,Relational>;
    using Context = Assemblies<Tiles,Relational == 3>;
    static constexpr int words = Tiles < 8 ? 8 / Tiles : 1;
    Word bank[words], slow[words];
    SynapticGenes genes{};
    bool multi{};
    AX_FN void develop(const Genome &g, Word tile) {
        if constexpr (Relational) Path::develop();
        if constexpr (Relational == 3) Context::reset_context();
        UNROLL
        for (int i = 0; i < words; ++i) {
            Word pool = tile + i*Tiles;
            bank[i] = pool < 8 ? developmental_contacts(g, pool) : 0;
            slow[i] = 0u - ((genes.conditional >> 22) & 1);
        }
    }
    AX_FN void load(const Genome &g, const Contacts *state, Word tile, bool fresh, const SynapticLayer *extra = nullptr,
                    const AssociationLayer *links = nullptr, const ContextLayer *contexts = nullptr,const ExpressionLayer *expression=nullptr,bool polarity=false) {
        if constexpr (Relational) Path::load(*links,tile,fresh);
        if constexpr (Relational == 3) Context::load_context(*contexts,tile,fresh,expression,polarity);
        multi = extra != nullptr;
        if (extra) genes = extra->genes;
        if (fresh) { develop(g, tile); return; }
        UNROLL
        for (int i = 0; i < words; ++i) {
            Word pool = tile + i*Tiles;
            bank[i] = pool < 8 ? state->weights[pool] : 0;
            slow[i] = multi && pool < 8 ? extra->state[pool] : 0;
        }
    }
    AX_FN void store(Contacts &state, Word tile, SynapticLayer *extra, AssociationLayer *links = nullptr) const {
        if constexpr (Relational) if (links) Path::store(*links,tile);
        UNROLL
        for (int i = 0; i < words; ++i) {
            Word pool = tile + i*Tiles;
            if (pool < 8) {
                state.weights[pool] = bank[i];
                if (extra) extra->state[pool] = slow[i];
            }
        }
    }
    AX_FN void update_pattern(Word rule, Word afferent, Word discharge, bool positive, Word tile, bool adjust_slow = true) {
        // The primary table address is (weight, earlier receptor, efference,
        // metabolic spike). The optional second plane selects another complete
        // table. Both outputs are computed from the same old synaptic state.
        UNROLL
        for (int i = 0; i < words; ++i) {
            Word offset = (positive ? 8 : 0) + (tile+i*Tiles == discharge ? 4 : 0);
            Word old = bank[i], next = lut2(rule >> offset, old, afferent);
            if (multi) {
                Word m = slow[i];
                Word other = lut2(genes.conditional >> offset, old, afferent);
                next = mux(m, other, next);
                if (adjust_slow) {
                    Word lo = lut2(genes.modulation >> offset, old, afferent);
                    Word hi = lut2((genes.modulation >> 16) >> offset, old, afferent);
                    slow[i] = mux(m, hi, lo);
                }
            }
            bank[i] = next;
        }
    }
    AX_FN void remember(Word stamp) { if constexpr (Relational == 3) Context::commit_context(stamp); }
    AX_FN bool stabilizes() const {
        if constexpr (Relational==3) return (Context::program.receptors&(1u<<28)) && Context::context_cut!=ContextCut::labile;
        return false;
    }
    AX_FN void observe(const Genome &g, Word rule, Word raw, Word projected, Word motor, bool positive, Word tile, bool adjust_slow = true, Word lag = 1) {
        if constexpr (Relational == 3) {
            Context::credit_program(lag);
            Word executed=motor;
            motor=Context::experience(g,raw,motor,positive,lag,tile);
            Word base=(index()&31)&~(Tiles-1), mask=Word((1ull<<Tiles)-1)<<base;
            Word candidates=stabilizes() ? Path::passage(raw,tile,base,mask,false).direct : 0;
            if (candidates && !(candidates&(candidates-1))) {
                // Reinstated fibre activity can teach an internal sequence
                // while its already selective physical contact is protected.
                bool pulse=Context::program.coordination&(1u<<29) ? positive : (Context::program.receptors>>26)&1;
                Path::update(g,raw,executed,pulse,tile,Word(__builtin_ctz(candidates)));
                return;
            }
            if (motor>=8 || !Context::trunk_open()) return;
        }
        update_pattern(rule,projected,motor,positive,tile,adjust_slow);
        if constexpr (Relational) Path::update(g,raw,motor,positive,tile);
    }
    AX_FN void update(const Genome &g, Word rule, Word previous, bool positive, Word tile, bool adjust_slow = true, Word lag = 1) {
        if (!previous) return;
        Word cue = earlier_cue(previous);
        observe(g,rule,pattern(cue),receptor(g,cue),(previous >> 5) & 7,positive,tile,adjust_slow,lag);
    }
    AX_FN Word drive(Word afferent, Word tile, Word base, Word mask) const {
        Word firing = 0;
        UNROLL
        for (int i = 0; i < words; ++i) {
            Word own = tile + i*Tiles;
            if (own < 8) {
                Word votes = population(bank[i] & afferent);
                if (multi) votes += ((genes.conditional >> 20) & 3) *
                    population(lut2(genes.conditional >> 16, bank[i], slow[i]) & afferent);
                firing |= lesser(15, votes) << (4*own);
            }
        }
        UNROLL
        for (int d = 1; d < Tiles; d *= 2) firing |= shuffle(firing, base + (tile ^ d), mask);
        return firing;
    }
    AX_FN Word read(const Genome &g, Word raw, Word projected, Word tile, Word base, Word mask, bool cut = false,
                    Word *connected = nullptr, Word ambient = 0, Word *stamp = nullptr,const MotorCounts *cortex=nullptr,Word tie=0) const {
        if constexpr (Relational==3) Context::sensory_program(raw,ambient);
        Word original = drive(projected,tile,base,mask);
        if constexpr (Relational) {
            Word direct=0, routed=Path::output(g,raw,tile,base,mask,cut,connected,Relational==3 ? &direct : nullptr), combined=0;
            UNROLL
            for (Word k=0; k<32; k+=4) combined |= lesser(15,((original >> k)&15) + ((routed >> k)&15)) << k;
            if constexpr (Relational == 3) {
                Word trace=stabilizes() && direct && !(direct&(direct-1)) ? Word(__builtin_ctz(direct)) : 4, mapped=0;
                if (trace<4) {
                    UNROLL
                    for (Word m=0;m<8;++m) {
                        Word physical=effector(g,m);
                        if (motor_tag(Path::genes,physical)==trace) mapped|=1u<<Context::ports(physical);
                    }
                }
                Word recalled=mapped && !(mapped&(mapped-1)) ? Word(__builtin_ctz(mapped)) : 4;
                return Context::express(g,raw,combined,ambient,tile,base,mask,stamp,cortex,tie,recalled);
            }
            else return combined;
        }
        return original;
    }
    AX_FN Word fire(const Genome &g, Word signature, Word tile, Word base, Word mask, bool cut = false,
                    Word *connected = nullptr, Word ambient = 0, Word *stamp = nullptr,const MotorCounts *cortex=nullptr,Word tie=0) const {
        return read(g,pattern(signature),receptor(g,signature),tile,base,mask,cut,connected,ambient,stamp,cortex,tie);
    }
};

struct PeripheralHistory {
    unsigned long long recent{}, older{};
    AX_FN void append(Word event) {
        older = (older << 16) | (recent >> 48);
        recent = (recent << 16) | event;
    }
    AX_FN Word at(Word lag) const {
        return Word((lag > 4 ? older : recent) >> (16*((lag-1) & 3))) & 65535;
    }
};
