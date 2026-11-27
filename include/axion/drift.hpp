#pragma once
#include "regulation.hpp"
#include "braid_view.hpp"
#include "braid_discourse.hpp"

namespace axion::life::drift {
enum Port : Word {
    moving = 1,
    tide = 2,
    recall = 4,
    plastic = 8,
    feedback = 16,
    building = 32,
    fibres = 64,
    contest = 128,
    swapped = 256,
    sham = 512,
    sight = 1024,
    audience = 2048,
    regrowth = 4096,
    rafting = 8192,
    lateral = 16384,
    misaligned = 32768,
    seepage = 65536,
    recruitment = 131072,
    lateral_contest = 262144,
    storage = 524288,
    auditory = 1048576,
    optical_channel = 3u << 21,
    optical_cadence = 7u << 23,
    bottom_depth = 1u << 26,
    tidal_banks = 1u << 27,
    visual_contest = 1u << 28,
    shoals = 1u << 29,
    winding = 1u << 30,
    independent_glints = 1u << 31,
    live = moving | tide | recall | plastic | feedback | building | fibres
};
struct Law {
    Word ports = 0, tide_shift = 6, stride_shift = 1, load = 12;
};
template <class Counter> struct Accounts {
    Counter pulses = 0, released = 0, eaten = 0, settled = 0, reached = 0, hit = 0, heat = 0, committed = 0,
            confirmed = 0, contact_edits = 0, somatic_edits = 0, learned_choices = 0, blank_eaten = 0, gained = 0,
            lost = 0, unbound_eaten = 0, fibre_diversions = 0, fibre_gain = 0, fibre_loss = 0, tied = 0, untied = 0,
            changed_routes = 0, crossed = 0, mobile_deposit = 0, young_reaches = 0, young_meals = 0, old_reaches = 0,
            old_meals = 0;
};
using Totals = Accounts<Count>;
using Pulse = Accounts<Word>;
struct Germline {
    regulation::Program program{};
    Word law = 0, wiring = 0, active = 1;
};
struct Diet {
    Word food = 0, blank = 0, phase[3]{}, changed = 0, changed_food = 0;
};
struct Headcount {
    Count alive = 0, young = 0, energy = 0, bits = 0, cells = 0;
};
struct Pedigree {
    Count births[3]{}, deaths[3]{};
};
struct State {
    Law law{};
    Totals total{};
    Pedigree pedigree{};
    Word tag_seed = 48191, marked_at = 0;
};
struct Intake {
    Count reached = 0, eaten = 0, without = 0, changed = 0, hits = 0, committed = 0, judged = 0, edits = 0;
};
struct Cargo {
    Count opportunities = 0, swaps = 0, blocked = 0, carbon = 0, travelled = 0;
};
struct Spill {
    Count inspected = 0, retained = 0, floods = 0, flood_stock = 0, flood_meals = 0, trickles = 0, trickle_stock = 0,
          trickle_meals = 0, held_by_fibres = 0, moved_rim = 0;
};
struct Signature {
    regulation::Program program{};
    Word hand = ~0u;
};
AX_FN Word lateral_hand(Word law, Word wiring, Word active) { return law | (active << 4) | (wiring << 5); }
AX_FN Word wiring(const braid::Genes &g, Word ports) {
    Word salt = (g.arbor >> 29) | ((g.muscle >> 30) << 3);
    return (g.plastic >> 20) | ((ports & recruitment) ? salt << 12 : 0u);
}
AX_FN void wire(braid::Genes &g, Word pattern, bool with_gain) {
    g.plastic = (g.plastic & 0xfffffu) | (pattern << 20);
    if (with_gain) {
        g.arbor = (g.arbor & 0x1fffffffu) | ((pattern >> 12) << 29);
        g.muscle = (g.muscle & 0x3fffffffu) | ((pattern >> 15) << 30);
    }
}
AX_FN bool matches(Signature wanted, regulation::Program organs, const braid::Genes &genes) {
    bool gain = wanted.hand >> 31;
    return organs.first == wanted.program.first && organs.second == wanted.program.second &&
           (wanted.hand == ~0u || (wanted.hand & 0x7fffffffu) ==
                                      lateral_hand(genes.plastic & 15, wiring(genes, gain ? Word(recruitment) : 0u),
                                                   (genes.arbor >> 12) & 1));
}
struct Courses {
    Count pulses = 0, serial = 0, stages = 0, order_changed = 0, beyond_first = 0, material = 0, eaten = 0,
          reversed_food = 0, reaches = 0, right = 0, single_right = 0, committed = 0, altered = 0, altered_map = 0;
};
struct Sampling {
    Intake *bins{};
    Signature donor{};
    Word has_donor = 0;
    Cargo *rafts{};
    Word aggregate = 0;
    Spill *spills{};
    Courses *courses{};
};
constexpr Word populations = 4, phases = 4;
AX_FN bool reads(Word ports, Word lineage) {
    if (!(ports & recall))
        return false;
    return !(ports & contest) || (ports & (sham | lateral_contest | visual_contest)) || !lineage ||
           lineage == ((ports & swapped) ? 2u : 1u);
}
AX_FN Word branch(Word ports, Word lineage) {
    if ((ports & lateral_contest) && lineage && lineage != ((ports & swapped) ? 2u : 1u))
        return ports & ~Word(lateral | recruitment | misaligned);
    return ports;
}

AX_FN bool valid(Law law) {
    return !(law.ports &
             ~Word(live | contest | swapped | sham | sight | audience | regrowth | rafting | lateral | misaligned |
                   seepage | recruitment | lateral_contest | storage | auditory | optical_channel | optical_cadence |
                   bottom_depth | tidal_banks | visual_contest | shoals | winding | independent_glints)) &&
           (!(law.ports & shoals) || (law.ports & bottom_depth)) &&
           (!(law.ports & visual_contest) || ((law.ports & contest) && !(law.ports & (sham | lateral_contest)))) &&
           (!(law.ports & lateral_contest) || ((law.ports & contest) && !(law.ports & sham))) &&
           (!(law.ports & (misaligned | recruitment)) || (law.ports & lateral)) &&
           (!(law.ports & (swapped | sham)) || (law.ports & contest)) &&
           (law.ports & (swapped | sham)) != (swapped | sham) && (!(law.ports & audience) || (law.ports & sight)) &&
           law.tide_shift <= 16 && law.stride_shift <= 3 && law.load >= 1 && law.load <= 64;
}
AX_FN bool balanced(const Totals &t) {
    return t.eaten <= t.released && t.settled == t.released - t.eaten && t.blank_eaten <= t.released &&
           t.unbound_eaten <= t.released && t.gained <= t.eaten && t.lost <= t.blank_eaten &&
           t.eaten - t.gained == t.blank_eaten - t.lost && t.fibre_gain <= t.eaten && t.fibre_loss <= t.unbound_eaten &&
           t.eaten - t.fibre_gain == t.unbound_eaten - t.fibre_loss && t.hit <= t.reached &&
           t.confirmed <= t.committed && t.committed <= t.reached && t.learned_choices <= t.reached &&
           t.crossed <= t.pulses && t.fibre_diversions <= t.pulses && t.mobile_deposit <= t.settled &&
           t.heat == t.reached + t.tied + t.untied && t.young_reaches + t.old_reaches == t.reached &&
           t.young_meals + t.old_meals == t.eaten;
}
struct Bed {
    flood::Half water[4]{}, clay[4]{}, fibre[4]{};
};
struct View {
    Word inlet = 0, phase = 0, rock = 0;
    Bed bed{};
};
AX_FN Word low(Word a, Word b) { return a < b ? a : b; }
struct Sill {
    Word site = 0, height = 0;
};
AX_FN Sill highest_sill(const View &v, bool bound = true) {
    Word rank = 0, first = v.rock >> 24;
#if defined(__CUDA_ARCH__)
#pragma unroll
#endif
    for (Word j = 0; j < 4; ++j) {
        Word previous = (j + 3) & 3;
        Word height = 8 + ((v.rock >> (4 * j)) & 15) + low(v.bed.clay[j] + v.bed.clay[previous], 12) +
                      (bound ? low(v.bed.fibre[j] + v.bed.fibre[previous], 8) : 0u);
        Word priority = 4 * height + 3 - ((j - first) & 3);
        rank = priority > rank ? priority : rank;
    }
    return {(first + 3 - (rank & 3)) & 3, rank >> 2};
}
AX_FN Word head(const View &v, Law law, Word tick, Word j, Word stock) {
    Word phase = ((tick + (v.rock >> 7)) >> (law.tide_shift > 5 ? law.tide_shift - 5 : 0)) & 31;
    Word tide_head = law.ports & tide ? (phase < 16 ? phase : 32 - phase) : 8u;
    Word water = (j & 2) ? ((j & 1) ? v.bed.water[3] : v.bed.water[2]) : ((j & 1) ? v.bed.water[1] : v.bed.water[0]);
    return low(water, 64u) / 4 + tide_head + low(stock, 32u) / 2;
}
// Positions are clockwise from the northwest corner. Opposite contacts in a
// layer operate on disjoint pairs. A later shifted block can move the same
// packet residue into a neighbouring catchment.
AX_FN Word connections(const View &v, bool conduct, bool seeps = false) {
    Word open = 0;
    for (Word e = 0; e < 4; ++e) {
        Word a = v.bed.water[e], b = v.bed.water[(e + 1) & 3];
        Word slope = a > b ? a - b : b - a;
        Word high = e & 1 ? (v.phase == 1 || v.phase == 2) : v.phase < 2;
        Word head = 1 + 6 * high + low(slope >> 2, 4);
        Word wick = conduct ? low(v.bed.fibre[e], 4u) * 2 : 0;
        Word resistance = 3 + ((v.rock >> (2 * e)) & 3) + low(v.bed.clay[e], 16);
        open |= Word(head + wick >= resistance) << e;
        // Water can cross a packed seam down the pressure gradient. A strong
        // head drop prevents the same open seam from conveying a return load.
        if (seeps && head + wick >= resistance && (v.bed.clay[e] || wick) && slope > 2 + ((v.rock >> (2 * e)) & 3))
            open |= 1u << (e + (a < b ? 4 : 8));
    }
    return open;
}
AX_FN Word horizontal(Word inlet, Word links) {
    Word east = inlet < 2 ? 0 : 2;
    return (links & (1u << east)) && !(links & (1u << (east + (inlet == east ? 4 : 8)))) ? inlet ^ 1u : inlet;
}
AX_FN Word outlet(Word inlet, Word links) {
    Word j = horizontal(inlet, links);
    Word south = j == 1 || j == 2 ? 1 : 3;
    return (links & (1u << south)) && !(links & (1u << (south + (j == south ? 4 : 8)))) ? (3u - j) : j;
}
// A section starts in the channel bed. Each fastened turn of cord takes the
// stream through the lower or upper set of seams once more. Crossing the
// four posts against the tide exposes the same material in the opposite order.
struct Path {
    Word lower = 0, upper = 0, turns = 0, length = 1;
    AX_FN Word run(Word src, bool reverse = false) const {
        for (Word j = 0; j < length; ++j) {
            Word k = reverse ? length - 1 - j : j;
            src = outlet(src, (turns & (1u << k)) ? upper : lower);
        }
        return src;
    }
    AX_FN Word shape() const { return lower | (upper << 12) | ((turns >> 1) << 24) | ((length - 1) << 28); }
    AX_FN static Path recover(Word packed) {
        return {packed & 4095u, (packed >> 12) & 4095u, ((packed >> 24) & 15u) << 1, 1 + ((packed >> 28) & 7u)};
    }
};
AX_FN Path woven_path(const View &v, Word floor, bool conduct, bool seeps) {
    Path p{floor};
    if (!conduct)
        return p;
    Word clips = 0, upper = 0;
    for (Word e = 0; e < 4; ++e) {
        Word next = (e + 1) & 3, a = v.bed.water[e], b = v.bed.water[next], fibre = v.bed.fibre[e],
             clay = v.bed.clay[e];
        Word notch = (v.rock >> (2 * e)) & 3;
        // Water wets a strand as capillary head exceeds the exposed rim.
        // A fastened bend can return a load even when another seam is dry.
        Word head = low(a < b ? a : b, 48u) / 8 + low(fibre, 8u);
        if (fibre && head >= 2 + notch + low(clay, 16u) / 2) {
            upper |= 1u << e;
            Word slope = a > b ? a - b : b - a;
            if (seeps && slope > 8 + 2 * low(fibre, 8u))
                upper |= 1u << (e + (a < b ? 4 : 8));
        }
        if (fibre && clay && fibre + low(clay, 4u) >= 3 + notch)
            clips |= 1u << e;
    }
    p.upper = upper;
    for (Word j = 0; j < 4; ++j) {
        Word post = ((v.rock >> 17) + ((v.phase & 2) ? 4 - j : j)) & 3;
        if (clips & (1u << post)) {
            Word lifted = (post + (v.rock >> 21)) & 1; // The two axes pass over and under their posts.
            p.turns |= lifted << p.length;
            ++p.length;
        }
    }
    return p;
}
AX_FN Word lateral_columns(Word gene, Word rows) {
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 1200
    // Each byte selects one truth-table bit for one outlet. PRMT gathers
    // the four bytes in parallel with selectors assembled from contact rows.
    auto spread = [](Word x) {
        x &= 15;
        x = (x | (x << 6)) & 0x0303u;
        return (x | (x << 3)) & 0x1111u;
    };
    Word indices = spread(rows >> 4) | (spread(rows >> 8) << 1) | (spread(rows >> 12) << 2);
    Word left = ((gene & 15) * 0x204081u) & 0x01010101u;
    Word right = (((gene >> 4) & 15) * 0x204081u) & 0x01010101u;
    Word columns;
    asm("prmt.b32 %0,%1,%2,%3;" : "=r"(columns) : "r"(left), "r"(right), "r"(indices));
    return ((columns * 0x01020408u) >> 24) & 15u;
#else
    return regulation::truth(gene, rows >> 4, rows >> 8, rows >> 12) & 15;
#endif
}
// Four distal dendrites per inlet see aligned bits in the other three
// contact banks. Both their truth table and their influence on the primary
// bank are inherited. No hydraulic geometry enters this neural readout.
AX_FN Word openings(Word contacts, Word row, Word gene, Word ports) {
    Word primary = (~contacts >> (4 * row)) & 15;
    if (!(ports & lateral))
        return primary;
    Word rows = ((contacts & 65535u) * 65537u) >> (4 * row);
    Word dendrites = lateral_columns(gene, rows);
    if (ports & recruitment)
        dendrites ^= (0u - ((gene >> (12 + __builtin_popcount(dendrites))) & 1u)) & 15;
    if (ports & misaligned)
        dendrites = ((dendrites << 1) | (dendrites >> 3)) & 15;
    Word gated = primary ^ (braid::rule_bits(gene >> 8, (~primary) & 15, dendrites) & 15);
    return gated ? gated : primary;
}
AX_FN Word face(Word axes, Word port) { return ((axes & 3) + ((axes & 4) ? 4 - port : port)) & 3; }
AX_FN Word choose(Word candidates, Word draw) {
    Word open = candidates ? candidates : 15u;
    Word rank = Word((Count(draw) * __builtin_popcount(open)) >> 32);
    while (rank--)
        open &= open - 1;
    return Word(__builtin_ctz(open));
}
struct Reach {
    Word id = 0, born = 0, row = 0, axes = 0, draw = 0, old = 0, command = 0, blank = 0, capacity = 0, bite = 0,
         after = 0, nerve = 0, background = 0, first = 0, second = 0, law = 0, issued = 0, learns = 0, spike_after = 0,
         tag = 0, uses = 0, wiring = 0, openings = 0;
    Word optical = 0, receptors = 0, visual_law = 0, optics = 0, heading = 0, hand = 0, a = 0, b = 0;
};
struct Scene {
    View view{};
    Word tick = 0, tile = 0, origin = 0, links = 0, unbound = 0, sink = 0, amount = 0, priority = 0, source_stock = 0,
         dest_litter = 0, same_stock = 0, after_stock = 0, after_litter = 0, eaten = 0, blank_eaten = 0,
         unbound_eaten = 0;
    Word rim = 0, height = 0, pressure = 0, held_stock = 0;
    Word upper = 0, turns = 0, laps = 1, resting = 0, dye = 0;
    Reach reach[4]{};
};
struct Heritage {
    Count placed = 0, revisited = 0, diverted = 0, actual = 0, without = 0, gained = 0, lost = 0, self = 0, foreign = 0,
          after_death = 0, younger = 0, orphans = 0, oldest = 0;
};
struct Inheritance {
    View view{};
    Mark maker{};
    Word side = 0, origin = 0, tile = 0, tick = 0, edge = 0, outlet = 0, alternative = 0, load = 0, priority = 0,
         aims = 0, capacities = 0, meals = 0, maker_now = 0, maker_energy = 0, rank = 0, who[4]{}, born[4]{};
    Word beneficiary = 0, shape = 0, limbs = 0, eyes = 0, pores = 0, cells = 0;
};
struct LegacyWatch {
    Heritage *counts{};
    Inheritance *samples{};
    Word limit = 0;
};
struct Exhibit {
    Word id = ~0u, born = 0, stamp = 0, tile = 0, inlet = 0, sink = 0, aim = 0, row = 0, old = 0, axes = 0,
         background = 0, dye = 0, retinal_places = 0, retinal_hues = 0, rafting = 0, towed = 0, raft_load = 0,
         water = 0;
    braid::Episode scene{};
    Word wiring = 0, choices = 0, local = 0, prior = 0, neighbours = 0, previous_reaches = 0, midpoint = 4;
};
struct Encounter {
    Exhibit view{};
    Word after = 0, command = 0, blank = 0, load = 0, bite = 0, available = 0;
};
struct Conversation {
    braid::Speech names{}, anchor{}, actual{}, before{}, inverse{};
    Word slot = 0;
};
struct Answers {
    Word turns[3][4]{};
};
struct Diary {
    Encounter *notes{};
    Word slot = 0, born = 0, start = 0, ticks = 0;
};
struct History {
    Word born = 0, visited = 0, reaches = 0, origin = 0, channels = 0, nearby = 0;
};
struct FirstChoices {
    Count total = 0, right = 0, local = 0, rotated = 0, changed = 0, certain = 0, certain_right = 0, food = 0,
          proximal_food = 0;
};
struct FirstWatch {
    History *visits{};
    FirstChoices *counts{};
    Word size = 0, start = 0, salt = 0;
};
struct Gaze {
    Exhibit *events{};
    Word *choice{};
    Word tiles = 0, kind = 0, selection = 0, eligible = 0;
    Signature target{};
    Diary diary{};
    FirstWatch first{};
};
AX_FN Word overlay(Word colors, Word port, Word dye) { return (colors & ~(3u << (2 * port))) | (dye << (2 * port)); }
AX_FN braid::Episode forecast_view(Word base, Word dye, Word source, Word options, Word axes, Word stamp, bool raft) {
    braid::Episode local{};
    local.stamp = stamp;
    braid::expose(local, braid::beginning, raft ? base : overlay(base, source, dye), 15);
    Word expected = 0, agree = 15;
    bool first = true;
    for (Word k = 0; k < 4; ++k)
        if (options & (1u << k)) {
            Word dest = face(axes, k);
            Word panel = raft ? braid::exchanged(base, source, dest) : overlay(base, dest, dye);
            if (first) {
                expected = panel;
                first = false;
            } else
                for (Word j = 0; j < 4; ++j)
                    if (braid::pigment(panel, j) != braid::pigment(expected, j))
                        agree &= ~(1u << j);
        }
    if (!first)
        braid::expose(local, braid::intended, expected, agree);
    return local;
}
AX_FN braid::Episode sighting(Word base, Word dye, Word source, Word sink, Word options, Word axes, Word stamp,
                              braid::Optics eye, bool raft = false, bool moved = false, Word midpoint = 4) {
    auto local = forecast_view(base, dye, source, options, axes, stamp, raft);
    braid::expose(local, braid::result,
                  raft ? (moved ? braid::exchanged(base, source, sink) : base) : overlay(base, sink, dye), 15);
    if (midpoint < 4)
        braid::expose(local, braid::shortened, overlay(base, midpoint, dye), 15);
    return braid::through_glass(local, eye);
}
struct Capture {
    Scene *records{};
    Word start = 0, ticks = 0, tiles = 0;
};
AX_FN Word strength(Word old, Word row, Word command, bool tasted, Word law, regulation::Source &organ, bool feedback,
                    bool structural, Word sensed = ~0u) {
    Word channel = 15u << (4 * row), touched = 1u << (4 * row + command);
    Word conflict = tasted ? channel ^ touched : touched;
    Word proposed = (old & ~channel) | (channel & braid::rule(law, old, conflict));
    Word available = sensed == ~0u ? ((~old >> (4 * row)) & 15) : sensed;
    bool committed = available && !(available & (available - 1));
    Word release = regulation::discharge(organ.program, organ.cells, old, conflict, proposed, feedback && committed,
                                         feedback && committed && tasted);
    Word next = regulation::candidate(organ.program, organ.cells.spike, old, proposed, structural);
    return regulation::apply(old, next, release & channel);
}
} // namespace axion::life::drift
