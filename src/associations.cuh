// The two banks receive physical or internal spike coincidences. Their
// four motor-tagged fibres use inherited Boolean plasticity tables.
struct LinkRow {
    Word low, high;
    AX_FN Word unique() const {
        Word a = low & 65535, b = low >> 16, c = high & 65535, d = high >> 16;
        return (a|b|c|d) & ~((a&b) | (c&d) | ((a|b)&(c|d)));
    }
    AX_FN Word code(Word bit) const {
        return Word(((low >> 16 | high >> 16) & bit) != 0) |
               (Word(((high | high >> 16) & bit) != 0) << 1);
    }
    AX_FN Word candidates(Word bit) const {
        return Word((low & bit) != 0) | (Word((low >> 16 & bit) != 0) << 1) |
               (Word((high & bit) != 0) << 2) | (Word((high >> 16 & bit) != 0) << 3);
    }
    AX_FN Word fibre(Word code) const {
        return ((code & 2 ? high : low) >> (16*(code & 1))) & 65535;
    }
    AX_FN Word overlap(LinkRow other) const {
        Word both = (low & other.low) | (high & other.high);
        return (both | (both >> 16)) & 65535;
    }
    AX_FN Word sites() const { return (low | high | (low >> 16) | (high >> 16)) & 65535; }
    AX_FN void excite(Word phase, Word sites) {
        Word firing = sites << (16*(phase & 1));
        if (phase & 2) high |= firing; else low |= firing;
    }
};
struct Passage { Word first{}, second{}, votes{}, direct{}, convergent{}; };
// Two thresholds of coincidence for 16 columns in parallel. Merging disjoint
// groups is associative, so both thresholds fit in a single shuffle word.
AX_FN Word sum_thresholds(Word x, Word y) {
    return (x | y) | ((x & y) << 16);
}
AX_FN Word convergence_cells(Word table, Word agree, Word differ) {
    Word a = agree, b = differ;
    Word lo = mux(agree >> 16, lut2(table >> 4,a,b), lut2(table,a,b));
    Word hi = mux(agree >> 16, lut2(table >> 12,a,b), lut2(table >> 8,a,b));
    return mux(differ >> 16,hi,lo) & 65535;
}

template<int Tiles>
AX_FN Word ensemble_or(Word value, Word tile, Word base, Word mask) {
    UNROLL
    for (int d=1; d<Tiles; d*=2) value |= shuffle(value,base + (tile ^ d),mask);
    return value;
}
AX_FN Word motor_tag(const LinkGenes &genes, Word ports) {
    return ((ports >> (genes.wiring & 7)) & 1) | (((ports >> ((genes.wiring >> 3) & 7)) & 1) << 1);
}
AX_FN LinkGenes link_founder(Word key) {
    // Two neutral plasticity tables; no physical or internal examples at birth.
    Word wiring = (mix(key+19) & 0xfbfff800u) | 8;
    return {mix(key+17) & 0x1ffffffu, 0xaaaaaaaau, wiring};
}
AX_FN LinkGenes inherit_links(LinkGenes genes, Word key, Word probability, bool mutate, Word &edits) {
    if (mutate) {
        Word a = mix(key ^ 0x765ac213u), b = mix(key ^ 0xe32c8b47u), c = mix(key ^ 0x472f9913u);
        if ((a & 255) < probability) { genes.switches ^= 1u << ((a >> 8) & 31); ++edits; }
        if ((b & 255) < probability) { genes.plastic ^= 1u << ((b >> 8) & 31); ++edits; }
        if ((c & 255) < probability) { genes.wiring ^= 1u << ((c >> 8) & 31); ++edits; }
    }
    return genes;
}
AX_FN Word inherit_convergence(Word table, Word key, Word probability, bool mutate, Word &edits) {
    Word draw = mix(key ^ 0x1be8a397u);
    if (mutate && (draw & 255) < probability) { table ^= 1u << ((draw >> 8) & 31); ++edits; }
    return table;
}

enum class PathControl : Word { intact, direct_only, silence_switches, silence_recall, fixed_pairs, fixed_recall, open_gates, no_convergence, fixed_range, short_range };
template<int Tiles, int Level>
struct Paths {
    static constexpr int row_count = Tiles < 16 ? 16/Tiles : 1;
    static constexpr int recall_count = Tiles < 8 ? 8/Tiles : 1;
    LinkRow rows[row_count];
    Word recalled[recall_count];
    LinkGenes genes;
    Word forward, backward, convergence;
    PathControl control = PathControl::intact;
    bool block_wave = false;

    AX_FN void develop() {
        Word initial = 0u - ((genes.wiring >> 25) & 1);
        UNROLL
        for (int j=0; j<row_count; ++j) rows[j] = {initial,initial};
        UNROLL
        for (int j=0; j<recall_count; ++j) recalled[j] = 0u - ((genes.wiring >> 26) & 1);
    }
    AX_FN void load(const AssociationLayer &stored, Word tile, bool fresh) {
        genes = stored.genes;
        convergence = stored.convergence;
        Word f = 0, r = 0;
        Word base = (index() & 31) & ~(Tiles-1), mask = Word((1ull << Tiles)-1) << base;
        UNROLL
        for (int j=0; j<row_count; ++j) {
            Word wire = tile + j*Tiles;
            if (wire < 16) {
                Word channel = wire >> 2, start = wire & 3, end = route_fibre(genes.switches,channel,start);
                f |= end << (2*wire);
                r |= start << (2*(4*channel+end));
            }
        }
        forward = ensemble_or<Tiles>(f,tile,base,mask);
        backward = ensemble_or<Tiles>(r,tile,base,mask);
        if (fresh) return;
        UNROLL
        for (int j=0; j<row_count; ++j) {
            Word row = tile+j*Tiles;
            rows[j] = row < 16 ? LinkRow{stored.rows[row][0],stored.rows[row][1]} : LinkRow{};
        }
        UNROLL
        for (int j=0; j<recall_count; ++j) {
            Word n = tile+j*Tiles;
            recalled[j] = n < 8 ? stored.recalled[n] : 0;
        }
    }
    AX_FN void store(AssociationLayer &dst, Word tile, Word dirty_rows = 65535, bool dirty_recall = true) const {
        UNROLL
        for (int j=0; j<row_count; ++j) {
            Word row = tile+j*Tiles;
            if (row < 16 && (dirty_rows & (1u << row))) {
                dst.rows[row][0] = rows[j].low; dst.rows[row][1] = rows[j].high;
            }
        }
        if (dirty_recall) {
            UNROLL
            for (int j=0; j<recall_count; ++j) if (tile+j*Tiles < 8) dst.recalled[tile+j*Tiles] = recalled[j];
        }
    }
    AX_FN void persist_event(AssociationLayer &dst, Word tile, Word previous, bool positive, bool fresh) const {
        if (fresh) { store(dst,tile); return; }
        if (!previous) return;
        Word rule = genes.plastic & 65535, dirty = 0;
        if (rule != 0xaaaa) {
            // When both motor conditions copy an inactive contact, only rows
            // lit by the earlier encounter could have changed. Other rules
            // retain full heterosynaptic access to every contact.
            Word event = rule >> (positive ? 8 : 0);
            dirty = (event & 0x33) == 0x22 ? rotate(pattern(earlier_cue(previous)),(genes.wiring>>6)&31) & 65535 : 65535;
        }
        store(dst,tile,dirty,(genes.plastic >> 16) != 0xaaaa);
    }
    AX_FN Word select_phase(Word table, Word code, Word phase) const { return (table >> (8*code+2*phase)) & 3; }
    // Each vertex has four phase cells and a transient refractory bit. Only
    // observed, unambiguous contacts conduct. A frontier is two lane registers.
    AX_FN Word propagate(Word raw, Word tile, Word base, Word mask, Word depth) const {
        Word cue = rotate(raw,(genes.wiring >> 6) & 31), left = cue & 65535, right = cue >> 16;
        if (!left || !right) return 0;
        left &= 0u-left; right &= 0u-right;
        Word start = (genes.wiring >> 11) & 3;
        LinkRow frontier{};
        frontier.excite(start,left);
        Word seen_left = left, seen_right = 0;
        for (Word step=1; step<=depth; ++step) {
            LinkRow next{};
            bool outgoing = step & 1;
            UNROLL
            for (int j=0; j<row_count; ++j) {
                Word row = tile+j*Tiles;
                if (row >= 16) continue;
                Word bit = 1u << row;
                Word phases = frontier.candidates(bit);
                if (outgoing ? !phases : bool(seen_left & bit)) continue;
                LinkRow edge = rows[j];
                Word available = edge.unique() & (outgoing ? ~seen_right : frontier.sites());
                if (!available) continue;
                UNROLL
                for (Word code=0; code<4; ++code) {
                    Word columns = edge.fibre(code) & available;
                    if (!columns) continue;
                    UNROLL
                    for (Word phase=0; phase<4; ++phase) {
                        Word end = select_phase(forward,code,phase);
                        if (outgoing) {
                            if (phases & (1u<<phase)) next.excite(end,columns);
                        } else if (columns & frontier.fibre(end)) next.excite(phase,bit);
                    }
                }
            }
            frontier.low = ensemble_or<Tiles>(next.low,tile,base,mask);
            frontier.high = ensemble_or<Tiles>(next.high,tile,base,mask);
            Word reached = frontier.sites();
            if (!reached) break;
            if (outgoing) {
                if (reached & right) {
                    Word arrival = frontier.candidates(right), response = 0;
                    UNROLL
                    for (Word code=0; code<4; ++code)
                        response |= ((arrival >> select_phase(forward,code,start)) & 1) << code;
                    return response;
                }
                seen_right |= reached;
            } else seen_left |= reached;
        }
        return 0;
    }
    AX_FN Passage passage(Word raw, Word tile, Word base, Word mask, bool traverse) const {
        Passage p{};
        Word sensory = rotate(raw,(genes.wiring >> 6) & 31);
        Word l = sensory & 65535, r = sensory >> 16;
        if (!l || !r) return p;
        Word s = __builtin_ctz(l), right = r & (0u-r);
        LinkRow mine{};
        UNROLL
        for (int j=0; j<row_count; ++j) if (s/Tiles == Word(j)) mine = rows[j];
        LinkRow origin{shuffle(mine.low,base+(s & (Tiles-1)),mask), shuffle(mine.high,base+(s & (Tiles-1)),mask)};
        p.direct = origin.candidates(right);
        if (!traverse || control == PathControl::direct_only) return p;
        Word active = origin.unique() & ~right;
        Word start = (genes.wiring >> 11) & 3;
        // A count reaching two also reached one. Seven truth-table addresses
        // per bank are unreachable and can encode readout traits.
        Word integrator = control == PathControl::no_convergence ? 0 : convergence & 0x8caf8cafu;
        Word agreement = 0, disagreement = 0;
        UNROLL
        for (int j=0; j<row_count; ++j) {
            Word node = tile+j*Tiles;
            LinkRow relay = rows[j];
            Word open = relay.unique(), shared = active & open;
            if (node >= 16 || node == s || !(right & open) || !shared) continue;
            Word c = relay.code(right);
            if (integrator) {
                Word matched = origin.overlap(relay) & shared;
                Word mismatched = shared ^ matched;
                Word address = Word(matched != 0) | (Word(mismatched != 0) << 1)
                             | (Word((matched & (matched-1)) != 0) << 2)
                             | (Word((mismatched & (mismatched-1)) != 0) << 3);
                p.convergent += ((integrator >> address) & 1) << (8*c);
                Word same = relay.fibre(c) & shared;
                agreement = sum_thresholds(agreement,same);
                disagreement = sum_thresholds(disagreement,shared ^ same);
            }
            Word gates = control == PathControl::open_gates ? 0 : genes.switches >> 25;
            if (gates & 7) {
                // Coincident firing on two of the three motor fibres can excite
                // or inhibit a candidate connection before the row spikes.
                Word first = shared;
                if (gates & 1) shared &= origin.overlap(relay) ^ (0u-((gates >> 3) & 1));
                if (gates & 2) shared &= relay.fibre(c) ^ (0u-((gates >> 4) & 1));
                if (gates & 4) shared &= origin.fibre(c) ^ (0u-((gates >> 5) & 1));
                if (!shared && (gates & 64)) shared = first;
            }
            if (shared) {
                // A single intermediate spike from this row carries its three
                // observed fibre identities. Exclude the cue under query.
                Word shift = (node + s + (genes.wiring >> 28)) & 15;
                Word shifted = ((shared | (shared << 16)) >> shift) & 65535;
                Word middle = 1u << ((Word(__builtin_ctz(shifted))+shift) & 15);
                Word a = origin.code(middle), b = relay.code(middle);
                Word event = a | (b << 2) | (c << 4);
                if (event < 32) p.first |= 1u << event; else p.second |= 1u << (event-32);
                Word phase = select_phase(forward,a,start);
                phase = select_phase(backward,b,phase);
                phase = select_phase(forward,c,phase);
                UNROLL
                for (Word k=0; k<4; ++k)
                    p.votes += Word(select_phase(forward,k,start) == phase) << (8*k);
            }
        }
        p.first = ensemble_or<Tiles>(p.first,tile,base,mask);
        p.second = ensemble_or<Tiles>(p.second,tile,base,mask);
        p.votes = group_sum<Tiles>(p.votes,tile,base,mask);
        if (integrator) {
            p.convergent = group_sum<Tiles>(p.convergent,tile,base,mask);
            UNROLL
            for (int d=1; d<Tiles; d*=2) {
                agreement = sum_thresholds(agreement,shuffle(agreement,base+(tile^d),mask));
                disagreement = sum_thresholds(disagreement,shuffle(disagreement,base+(tile^d),mask));
            }
            Word spikes = convergence_cells(integrator >> 16,agreement,disagreement) & active;
            UNROLL
            for (Word c=0; c<4; ++c) p.convergent += Word(population(spikes & origin.fibre(c))) << (8*c);
        }
        return p;
    }
    AX_FN void update(const Genome &g, Word raw, Word motor, bool positive, Word tile,Word reinstated_code=4) {
        Word field = rotate(raw,(genes.wiring >> 6) & 31);
        Word code = motor_tag(genes,effector(g,motor));
        Word edge_rule = genes.plastic & 65535, replay_rule = genes.plastic >> 16;
        if (reinstated_code<4) code=reinstated_code;
        if (replay_rule != 0xaaaau && control != PathControl::fixed_recall) {
            Word base = (index() & 31) & ~(Tiles-1), mask = Word((1ull << Tiles)-1) << base;
            auto p = passage(raw,tile,base,mask,true);
            UNROLL
            for (int j=0; j<recall_count; ++j) {
                Word n = tile+j*Tiles;
                Word table = replay_rule >> ((positive ? 8 : 0) + (n/2 == code ? 4 : 0));
                recalled[j] = lut2(table,recalled[j],n & 1 ? p.second : p.first);
            }
        }
        if (reinstated_code<4 || edge_rule == 0xaaaau || control == PathControl::fixed_pairs) return;
        UNROLL
        for (int j=0; j<row_count; ++j) {
            Word row = tile+j*Tiles;
            Word pre = row < 16 && ((field >> row) & 1) ? (field >> 16) : 0;
            LinkRow &old = rows[j];
            Word table = edge_rule >> (positive ? 8 : 0);
            Word x = lut2(table >> (code == 0 ? 4 : 0), old.low, pre) & 65535;
            Word y = lut2(table >> (code == 1 ? 4 : 0), old.low >> 16, pre) & 65535;
            Word z = lut2(table >> (code == 2 ? 4 : 0), old.high, pre) & 65535;
            Word w = lut2(table >> (code == 3 ? 4 : 0), old.high >> 16, pre) & 65535;
            old = {x | (y << 16), z | (w << 16)};
        }
    }
    AX_FN Word output(const Genome &g, Word raw, Word tile, Word base, Word mask, bool cut, Word *connected, Word *physical=nullptr) const {
        Word hop_gain = control == PathControl::silence_switches ? 0 : (genes.wiring >> 17) & 15;
        Word recall_gain = control == PathControl::silence_recall ? 0 : (genes.wiring >> 21) & 15;
        Word integration_gain = (genes.wiring >> 13) & 15;
        auto p = passage(raw,tile,base,mask,!cut && ((hop_gain | recall_gain) || (convergence && integration_gain) || connected));
        if (physical) *physical=p.direct;
        Word wave = 0, wave_gain = 0;
        if constexpr (Level >= 2) {
            Word pulses = (convergence >> 8) & 3;
            if (pulses && !block_wave && !cut && control != PathControl::direct_only &&
                control != PathControl::short_range && control != PathControl::silence_switches &&
                population(p.direct) != 1 && (!(p.first | p.second) || (convergence & (1u<<20)))) {
                wave = propagate(raw,tile,base,mask,(4u<<pulses)-1);
                wave_gain = 2*(1+((convergence >> 12) & 7));
            }
        }
        if (connected) *connected = (p.first | p.second | p.convergent | wave) != 0;
        Word learned = 0;
        if (recall_gain && (p.first | p.second)) {
            UNROLL
            for (int j=0; j<recall_count; ++j) {
                Word n = tile+j*Tiles;
                if (n < 8) learned += Word(population(recalled[j] & (n & 1 ? p.second : p.first))) << (8*(n/2));
            }
            learned = group_sum<Tiles>(learned,tile,base,mask);
        }
        Word strength[4], peak = 0;
        bool direct_wins = (genes.wiring & (1u<<27)) && population(p.direct) == 1;
        UNROLL
        for (Word c=0; c<4; ++c) {
            Word direct = ((p.direct >> c) & 1) * integration_gain;
            Word via = ((p.votes >> (8*c)) & 255)*hop_gain + ((learned >> (8*c)) & 255)*recall_gain
                     + ((p.convergent >> (8*c)) & 255)*integration_gain + ((wave >> c) & 1)*wave_gain;
            strength[c] = direct + (direct_wins ? 0 : via);
            if (strength[c] > peak) peak = strength[c];
        }
        Word mode = control == PathControl::fixed_range ? 0 : ((convergence >> 4) & 1) | ((convergence >> 5) & 2);
        Word offset = peak > 15 ? peak-15 : 0;
        Word scale = (31 - Word(__builtin_clz(peak | 15))) - 3;
        Word levels = 0;
        UNROLL
        for (Word c=0; c<4; ++c) {
            Word x = strength[c];
            if (mode == 1) x -= lesser(x,offset);
            if (mode == 2) x >>= scale;
            if (mode == 3 && x != peak) x = 0;
            levels |= lesser(15u,x) << (4*c);
        }
        Word vote = 0;
        UNROLL
        for (Word m=0; m<8; ++m) vote |= ((levels >> (4*motor_tag(genes,effector(g,m)))) & 15) << (4*m);
        return vote;
    }
    AX_FN bool different(const Paths &other, Word tile) const {
        Word diff=0;
        UNROLL
        for (int j=0; j<row_count; ++j) if (tile+j*Tiles < 16)
            diff |= (rows[j].low ^ other.rows[j].low) | (rows[j].high ^ other.rows[j].high);
        UNROLL
        for (int j=0; j<recall_count; ++j) if (tile+j*Tiles < 8) diff |= recalled[j] ^ other.recalled[j];
        return diff != 0;
    }
};
template<int Tiles> struct Paths<Tiles,0> {};
