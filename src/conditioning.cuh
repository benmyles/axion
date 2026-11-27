// Counterbalanced closed-loop test of the same motor populations, somatic
// transition and dendritic updates used during a life in the shared habitat.
namespace {
enum class Feedback : Word { intact, yoked, fixed_contacts, scrambled_motor };

AX_FN Word panel_tag(Word key, Word variety) {
    return ((key >> 16) + variety * ((key >> 8) | 1)) & 31;
}
AX_FN Word panel_grip(Word key, Word twist, Word variety, bool reversed) {
    Word private_key = mix(key ^ 0xf6c9583bu ^ (variety * 0x94d049bbu));
    Word offset = reversed ? 1 + ((private_key >> 8) % 3) : 0;
    return (private_key + twist + offset) & 3;
}
AX_FN bool consumes(const Genome &g, Word motor, Word target) {
    return (effector(g, motor) & 11) == (8 | target);
}

struct PanelItem {
    Word shape, texture;
    AX_FN Word cue(Word key, bool paired) const {
        if (!paired) return panel_tag(key, shape);
        return pair_cue(attribute(key, shape, 0), attribute(mix(key), texture, 1));
    }
    AX_FN Word target(Word key, Word twist, bool shifted, bool paired, bool composition, bool alternate, JointLaw law) const {
        if (!paired) {
            bool changed = ((shape + (key >> 23)) & 1) != 0;
            return panel_grip(key, twist, shape, shifted && changed);
        }
        if (composition) {
            Word left = panel_grip(key ^ 0x902a6713u, 0, shape, false);
            bool change = shifted && law != JointLaw::changing && ((texture + (key >> 23)) & 1);
            Word right = panel_grip(key ^ 0x43d91b85u, 0, texture, change);
            if (law == JointLaw::mosaic) {
                Word family = (mix(key ^ 0x1073ffe5u) + 2*Word(alternate)) & 3;
                if (family < 2) law = JointLaw::private_code;
                else law = family == 2 ? JointLaw::kinds : JointLaw::transposed;
            }
            if (law == JointLaw::independent) {
                Word k = mix(key ^ 0xcd386e2bu ^ (shape*0xb1df4157u) ^ (texture*0xc2b2ae35u));
                return (k + twist + (change ? 1+((k >> 8)%3) : 0)) & 3;
            }
            if (law == JointLaw::latent_table) {
                // Private, generally non-invertible responses. Multiple visible
                // items on each axis share a kind. One agreement can be accidental.
                Word table = mix(key ^ 0x31975bebu);
                return ((table >> (2*(4*left + right))) + twist) & 3;
            }
            if (law == JointLaw::kinds || law == JointLaw::transposed) {
                // An unrelated four-way actuator permutation in every visible
                // context. Items can still share an unobserved functional kind.
                bool swap = law == JointLaw::transposed;
                Word context = swap ? texture : shape;
                Word kind = swap ? panel_grip(key ^ 0x902a6713u,0,shape,shifted && ((shape+(key>>23)) & 1)) : right;
                Word perm = permutation(mix(key ^ 0x5bd1e995u ^ (context*0x9e3779b9u)),0);
                return ((perm >> (2*kind)) + twist) & 3;
            }
            if (law == JointLaw::parity || law == JointLaw::parity_chains) return ((left ^ right) + twist) & 3;
            if (law == JointLaw::private_code || law == JointLaw::changing) {
                // Cross the same inputs and donor with two incompatible laws.
                // Only the simulator calculating consequences sees these bits.
                bool other = law == JointLaw::changing && shifted;
                bool parity = bool(mix(key ^ 0x91e10da5u) & 1) != (alternate != other);
                Word k = parity ? (left ^ right) : (left + right) & 3;
                Word perm = permutation(mix(key ^ (other ? 0x736a20bdu : 0x2342a697u)),0);
                return ((perm >> (2*k)) + twist) & 3;
            }
            return (left + right + twist) & 3;
        }
        bool use_texture = bool(key & 1) != (shifted != alternate);
        Word assignment = key ^ (use_texture ? 0x43d91b85u : 0x902a6713u);
        return panel_grip(assignment, twist, use_texture ? texture : shape, false);
    }
};
AX_FN PanelItem present(Word key, Word step, Conditioning cfg) {
    Word round = step / cfg.varieties;
    Word order = mix(key ^ (round * 0x85ebca6bu) ^ 0x4b765dedu);
    Word shape = ((order >> 16) + (order | 1) * step) & (cfg.varieties - 1);
    Word offset = (round + (key >> 17)) & (practice_width(cfg)-1);
    if (cfg.law == JointLaw::islands) offset *= 2;
    return {shape, (shape + offset) & (cfg.varieties - 1)};
}

template<int Tiles, int Relational>
AX_FN Word take_action(const Genome &g, Tissue &state, const Dendrites<Tiles,Relational> &synapses, Word tag,
                       Word recent, Word tie, Word tile, Word base, Word mask, Settings c, bool cut = false,
                       Word *connected = nullptr, Word sensory = (7u << 16) | (1u << 24), Word *stamp = nullptr,
                       const Prospects<Tiles> *prospects=nullptr,Word need=0,const MapCircuit<Tiles> *map=nullptr,Word place=0) {
    sensory^=place;
    Word ambient=sensory;
    if (recent) sensory |= 1u << (recent == 1 ? 22 : 23);
    sensory ^= pattern(tag);
    neural<Tiles, false>(g, state, sensory, tile, base, mask, c.plastic);
    if constexpr (Relational!=3) if (c.random_actions) return tie & 7;
    auto votes=motor_counts<Tiles>(g,state,tile,base,mask);
    if (c.habitat != Habitat::meadow) {
        Word firing = synapses.fire(g, tag, tile, base, mask, cut, connected,ambient,stamp,&votes,tie);
        if (prospects) firing=prospects->drive(g,pattern(tag),need,firing,tile,base,mask);
        if (map) firing=map->drive(g,place,0,need,firing,tile,base,mask,c.navigation,tie,&votes);
        if constexpr (Relational==3) if (c.random_actions) return tie & 7;
        return select_motor<true>(g,votes,tie,firing);
    }
    return select_motor<false>(g,votes,tie);
}

// Express every sampled germline against the same pair of histories. Keep
// the full physical effector byte at every probe, including non-feeding acts.
template<int Tiles, int Relational>
AX_FN void survey(const Site *world, const Genome *genes, const Word *positions, Word survivors, Word samples, Word seed,
                  Settings c, const SynapticLayer *layers, const AssociationLayer *links, BehavioralSample *out) {
    Word id = index(), trial = id/Tiles, tile = id & (Tiles-1), arm = trial & 1, sample = trial/2;
    if (sample >= samples) return;
    Word rank = Word(Count(sample)*survivors/samples);
    Word site = positions[(rank + mix(seed)%survivors)%survivors], slot = world[site].body.slot;
    const Genome &g = genes[slot];
    Word base = (id & 31) & ~(Tiles-1), mask = Word((1ull<<Tiles)-1) << base;
    Tissue tissue = develop(mix(g.development+tile*0x9e3779b9u));
    Dendrites<Tiles,Relational> synapses;
    synapses.load(g,nullptr,tile,true,layers ? &layers[slot] : nullptr,Relational ? &links[slot] : nullptr,
                  Relational==3 ? context_tail(links,c.side*c.side)+slot : nullptr,
                  Relational==3 && c.programs==2 ? expression_tail(links,c.side*c.side)+slot : nullptr,c.polarity);
    Word lag = c.digestion ? conduction(g) : 1, delay = c.digestion ? c.digestion : 1;
    Word recent = 0, outcomes = 0, decoder = permutation(mix(seed^0x937fc381u),0);
    PeripheralHistory history{};
    for (Word step=0; step<64; ++step) {
        Word order = mix(seed ^ ((step/16)*0xf671226bu));
        Word pair = ((order>>16) + (order|1)*step) & 15;
        PanelItem item{pair & 3,pair >> 2};
        Word cue = item.cue(seed,true), tie = mix(seed + step*0x632bd34bu);
        Word stamp=0;
        Word motor = take_action<Tiles>(g,tissue,synapses,cue,recent,tie,tile,base,mask,c,false,nullptr,0x01070000u,&stamp);
        synapses.remember(stamp);
        Word target = (decoder >> (2*(arm ? item.texture : item.shape))) & 3;
        outcomes = (outcomes<<1) | Word(consumes(g,motor,target));
        history.append(delayed(cue,motor));
        if (step+1 >= delay) {
            bool gain = (outcomes>>(delay-1)) & 1;
            recent = gain ? 1 : 2;
            if (step+1 >= lag && c.plastic && c.habitat != Habitat::meadow)
                synapses.update(g,g.expression>>16,history.at(lag),gain,tile,true,lag);
        }
    }
    for (Word i=0; i<3; ++i) neural<Tiles,false>(g,tissue,1u<<24,tile,base,mask,c.plastic);
    Word packed = 0;
    for (Word probe=0; probe<16; ++probe) {
        PanelItem item{probe & 3,probe >> 2};
        Word cue = item.cue(seed,true), heading = probe & 3;
        Word field = (1u<<heading) | (1u<<(24+(probe>>2))) | (probe&1 ? (3u<<16) : (1u<<19));
        if (probe & 2) field |= 1u<<(4+heading);
        if (probe & 4) field |= 1u<<30;
        Tissue copy = tissue;
        Word motor = take_action<Tiles>(g,copy,synapses,cue,0,mix(seed^probe),tile,base,mask,c,false,nullptr,field);
        packed |= Word(effector(g,motor)) << (8*(probe&3));
        if ((probe & 3) == 3) {
            if (!tile) out[sample].response[arm][probe/4] = packed;
            packed = 0;
        }
    }
    if (!tile && !arm) out[sample].site = site;
}

template<int Tiles, bool Select = false, int Relational = 0>
AX_FN void panel_probe(const Genome &g, Tissue state, const Dendrites<Tiles,Relational> &synapses,
                       Word key, Word twist, Word phase, Word tile, Word base, Word mask,
                       Settings c, Conditioning cfg, ConditioningScore &score, bool alternate) {
    // The probe presents an uninformative interval without contact or an
    // energy outcome. Independent copies answer each item from the same state.
    for (int delay = 0; delay < 3; ++delay)
        neural<Tiles, false>(g, state, 1u << 24, tile, base, mask, c.plastic);
    Dendrites<Tiles,Relational> initial = synapses;
    initial.develop(g, tile);
    Word pairs = probe_width(cfg), support = cfg.dimensions ? practice_width(cfg) : 1;
    for (Word v = 0; v < cfg.varieties; ++v) {
        for (Word offset = 0; offset < pairs; ++offset) {
            // Complementary diagonals keep the test pairs disjoint from every
            // acquisition/reversal encounter. Both dimensions remain balanced.
            Word delta = cfg.law == JointLaw::islands ? 2*offset+1 : support+offset;
            PanelItem item{v, (v + delta) & (cfg.varieties - 1)};
            Word original = item.target(key, twist, false, cfg.dimensions, cfg.composition, alternate, cfg.law);
            Word replacement = item.target(key, twist, true, cfg.dimensions, cfg.composition, alternate, cfg.law);
            Word tag = item.cue(key, cfg.dimensions), tie = mix(key ^ 0xe08f6be4u ^ (v + 32*offset));
            Tissue copy = state;
            Word evidence = 0;
            Word motor = take_action<Tiles>(g,copy,synapses,tag,0,tie,tile,base,mask,c,false,
                                            !Select && Relational && phase == 1 ? &evidence : nullptr);
            bool changed = original != replacement;
            if (phase == 0) {
                score.naive += consumes(g, motor, original);
                score.changed_items += changed;
                Word possible = 0;
                UNROLL
                for (Word a = 0; a < 8; ++a) possible |= consumes(g, a, original);
                score.reachable += possible;
            }
            if (phase == 1) {
                score.learned += consumes(g, motor, original);
                if (changed) score.surprise += consumes(g, motor, replacement);
                if constexpr (!Select) {
                    if (chain_law(cfg.law) && delta > 2 && delta < cfg.varieties-1) {
                        ++score.distant_items;
                        score.distant_correct += consumes(g,motor,original);
                    }
                    bool remote = chain_law(cfg.law) && delta > 4 && delta < cfg.varieties-3;
                    if (remote) { ++score.remote_items; score.remote_correct += consumes(g,motor,original); }
                    if constexpr (Relational >= 2) {
                        auto confined = synapses; confined.block_wave = true;
                        copy = state;
                        Word local = take_action<Tiles>(g,copy,confined,tag,0,tie,tile,base,mask,c);
                        bool success = consumes(g,local,original);
                        score.local_readout += success;
                        if (remote) score.remote_local += success;
                    }
                    if (panel_variants(cfg) == 8) {
                        Word other = item.target(key,twist,false,cfg.dimensions,cfg.composition,!alternate,cfg.law);
                        if (other != original) {
                            ++score.discordant_items;
                            score.discordant_correct += consumes(g,motor,original);
                            score.alternate_correct += consumes(g,motor,other);
                        }
                    }
                    if constexpr (Relational) {
                        copy = state;
                        Word direct = take_action<Tiles>(g,copy,synapses,tag,0,tie,tile,base,mask,c,true);
                        score.readout_cut += consumes(g,direct,original);
                        score.connected += evidence;
                    }
                    copy = state;
                    Word erased = take_action<Tiles>(g, copy, initial, tag, 0, tie, tile, base, mask, c);
                    score.erased += consumes(g, erased, original);
                    if (cfg.dimensions && offset < support && ((offset - (key >> 17)) & (support-1)) < cfg.exposures) {
                        PanelItem seen{v, (v + (cfg.law == JointLaw::islands ? 2*offset : offset)) & (cfg.varieties - 1)};
                        copy = state;
                        Word familiar = take_action<Tiles>(g, copy, synapses, seen.cue(key, true), 0, tie, tile, base, mask, c);
                        score.practiced += consumes(g, familiar, seen.target(key, twist, false, true, cfg.composition, alternate, cfg.law));
                    } else if (!cfg.dimensions) score.practiced += consumes(g, motor, original);
                }
            }
            if (phase == 2 && changed) score.one += consumes(g, motor, replacement);
            if (phase == 3) {
                if (changed) score.recovered += consumes(g, motor, replacement);
                else score.retained += consumes(g, motor, original);
            }
        }
    }
}

template<int Tiles, bool Select = false, int Relational = 0>
AX_FN void conditioning(const Genome *genes, const Word *living, Word survivors, Word *trace,
                        ConditioningScore *totals, Settings c, Conditioning cfg, Feedback arm,
                        Word first, Word count, const SynapticLayer *layers, Aptitude *fitness = nullptr, const AssociationLayer *links = nullptr) {
    Word id = index(), in_batch = id / Tiles, tile = id & (Tiles - 1);
    if (in_batch >= count) return;
    Word variants = Select ? 4 : panel_variants(cfg);
    Word trial = first + in_batch, group = trial / variants, twist = trial & 3;
    // The second quartet sees identical cues, donor, tie breaks and order.
    // Only which property produces the outcome changes. A yoked donor always
    // comes from a different set, preserving the complete outcome marginal.
    bool alternate = (trial % variants) >= 4;
    Word slot = 0;
    if constexpr (Select) { slot = group / cfg.quartets; group %= cfg.quartets; }
    Word key = mix(cfg.seed ^ (group * 0x9e3779b9u)), base = (id & 31) & ~(Tiles - 1);
    Word mask = Word((1ull << Tiles) - 1) << base;
    if constexpr (!Select) slot = living[mix(key ^ 0x9b74d43fu) % survivors];
    const Genome &g = genes[slot];
    Word rule = g.expression >> 16;
    if (Word(arm) >= 4 && Word(arm) < 20) {
        Word bit = 1u << (Word(arm)-4);
        rule = (rule & ~bit) | (0xaaaau & bit);
    }
    Tissue state = develop(mix(g.development + 0x9e3779b9u * tile));
    Dendrites<Tiles,Relational> synapses;
    synapses.load(g, nullptr, tile, true, layers ? &layers[slot] : nullptr, Relational ? &links[slot] : nullptr,
                  Relational==3 ? context_tail(links,c.side*c.side)+slot : nullptr,
                  Relational==3 && c.programs==2 ? expression_tail(links,c.side*c.side)+slot : nullptr,c.polarity);
    if constexpr (Relational) if (Word(arm) >= 77) synapses.control = PathControl(Word(arm)-76);
    // The upper primary table is used when the second state bit is set.
    // The separate 32-row table writes that bit from the same old state.
    if (Word(arm) >= 29 && Word(arm) < 45) {
        Word bit = 1u << (Word(arm)-29);
        synapses.genes.conditional = (synapses.genes.conditional & ~bit) | (0xaaaau & bit);
    } else if (Word(arm) >= 45 && Word(arm) < 77) {
        Word bit = 1u << (Word(arm)-45);
        synapses.genes.modulation = (synapses.genes.modulation & ~bit) | (0xffff0000u & bit);
    }
    ConditioningScore score{};
    if constexpr (!Select) panel_probe<Tiles>(g, state, synapses, key, twist, 0, tile, base, mask, c, cfg, score, alternate);
    Word n = cfg.varieties * cfg.exposures, stride = (2*n + 31) / 32;
    Word partner = variants*((group + 1) % cfg.quartets) + trial % variants;
    Word recent = 0, packed = 0, outcomes = 0;
    Word lag = c.digestion ? conduction(g) : 1;
    if (Word(arm) >= 20 && Word(arm) < 28) lag = Word(arm)-19;
    PeripheralHistory history{};
    for (Word step = 0; step < 2*n; ++step) {
        PanelItem item = present(key, step, cfg);
        Word tag = item.cue(key, cfg.dimensions), tie = mix(key ^ step * 0xc2b2ae35u ^ 0x55decaedu);
        Word stamp=0;
        Word motor = take_action<Tiles>(g,state,synapses,tag,recent,tie,tile,base,mask,c,false,nullptr,0x01070000u,&stamp);
        synapses.remember(stamp);
        bool success = consumes(g, motor, item.target(key, twist, step >= n, cfg.dimensions, cfg.composition, alternate, cfg.law));
        score.successes += success;
        score.attempts += (effector(g, motor) & 8) != 0;
        if (!Select && arm == Feedback::intact) {
            packed |= Word(success) << (step & 31);
            if ((step & 31) == 31 || step + 1 == 2*n) {
                if (!tile) trace[trial*stride + step/32] = packed;
                packed = 0;
            }
        }
        Word discharge = arm == Feedback::scrambled_motor ? mix(tie ^ 0x82bed45bu) & 7 : motor;
        history.append(delayed(tag, discharge));
        outcomes = (outcomes << 1) | Word(success);
        recent = 0;
        if (step+1 >= cfg.delay) {
            Word origin = step+1-cfg.delay;
            bool signal = (outcomes >> (cfg.delay-1)) & 1;
            if (arm == Feedback::yoked) signal = (trace[partner*stride + origin/32] >> (origin & 31)) & 1;
            score.delivered += signal;
            recent = signal ? 1 : 2;
            if (c.habitat != Habitat::meadow && c.plastic && arm != Feedback::fixed_contacts && step+1 >= lag) {
                Word selected = history.at(lag);
                synapses.update(g, rule, selected, signal, tile, Word(arm) != 28,lag);
            }
        }
        if (step+1 == n) panel_probe<Tiles,Select>(g, state, synapses, key, twist, 1, tile, base, mask, c, cfg, score, alternate);
        if (!Select && step+1 == n+cfg.varieties) panel_probe<Tiles>(g, state, synapses, key, twist, 2, tile, base, mask, c, cfg, score, alternate);
    }
    panel_probe<Tiles,Select>(g, state, synapses, key, twist, 3, tile, base, mask, c, cfg, score, alternate);
    if (!tile) {
        if constexpr (Select) {
            auto *s = &fitness[slot].score[cfg.fitness_offset];
            Word items = cfg.varieties*probe_width(cfg);
            Word adapted = Word(cfg.dimensions ? score.recovered + score.retained : 2*score.recovered) * 128 / items;
            asm volatile("red.global.add.u32 [%0], %1;" :: "l"(s), "r"(Word(score.learned)*128/items) : "memory");
            asm volatile("red.global.add.u32 [%0], %1;" :: "l"(s+1), "r"(adapted) : "memory");
            return;
        }
        auto &r = totals[Word(arm)];
#define SCORE(field) asm volatile("red.global.add.u64 [%0], %1;" :: "l"(&r.field), "l"(score.field) : "memory")
        SCORE(naive); SCORE(learned); SCORE(erased); SCORE(surprise); SCORE(one);
        SCORE(recovered); SCORE(retained); SCORE(attempts); SCORE(successes); SCORE(delivered); SCORE(changed_items); SCORE(practiced); SCORE(reachable); SCORE(readout_cut); SCORE(connected);
        SCORE(discordant_items); SCORE(discordant_correct); SCORE(alternate_correct);
        SCORE(distant_items); SCORE(distant_correct);
        SCORE(remote_items); SCORE(remote_correct); SCORE(local_readout); SCORE(remote_local);
#undef SCORE
    }
}
} // namespace

#if defined(AX_FAMILY_assays)
#define REL_PANEL(n, prefix, level) \
    ENTRY void prefix##_condition##n(const Genome *g, const Word *living, Word survivors, Word *trace, ConditioningScore *totals, Settings c, Conditioning cfg, Feedback arm, Word first, Word count, const SynapticLayer *layers, const AssociationLayer *links) { conditioning<n,false,level>(g,living,survivors,trace,totals,c,cfg,arm,first,count,layers,nullptr,links); } \
    ENTRY void prefix##_audition##n(const Genome *g, Aptitude *fitness, Settings c, Conditioning cfg, Word first, Word count, const SynapticLayer *layers, const AssociationLayer *links) { conditioning<n,true,level>(g,nullptr,0,nullptr,nullptr,c,cfg,Feedback::intact,first,count,layers,fitness,links); }
#define PANEL(n) ENTRY void condition##n(const Genome *g, const Word *living, Word survivors, Word *trace, ConditioningScore *totals, Settings c, Conditioning cfg, Feedback arm, Word first, Word count, const SynapticLayer *layers) { conditioning<n>(g, living, survivors, trace, totals, c, cfg, arm, first, count, layers); } \
    ENTRY void audition##n(const Genome *g, Aptitude *fitness, Settings c, Conditioning cfg, Word first, Word count, const SynapticLayer *layers) { conditioning<n,true>(g, nullptr, 0, nullptr, nullptr, c, cfg, Feedback::intact, first, count, layers, fitness); } \
    REL_PANEL(n, linked, 1) \
    REL_PANEL(n, wave, 2) \
    REL_PANEL(n, context, 3)
PANEL(2)
PANEL(4)
PANEL(8)
PANEL(16)
PANEL(32)
#undef PANEL

#define SURVEY(n, level) ENTRY void survey##level##_##n(const Site *w, const Genome *g, const Word *positions, Word alive, Word samples, Word seed, Settings c, const SynapticLayer *layers, const AssociationLayer *links, BehavioralSample *out) { survey<n,level>(w,g,positions,alive,samples,seed,c,layers,links,out); }
#define SURVEYS(n) SURVEY(n,0) SURVEY(n,1) SURVEY(n,2) SURVEY(n,3)
SURVEYS(2)
SURVEYS(4)
SURVEYS(8)
SURVEYS(16)
SURVEYS(32)
#undef SURVEYS
#undef SURVEY


#include "breeding.cuh"
#endif
