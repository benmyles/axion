// Four operators can intercept a single real suspended load. A block finishes
// all commitments before it admits any tactile evidence to a learning circuit.
namespace {
// Explicit multiplexers keep four neighbouring addresses in registers on
// Blackwell even when a creature points at a dynamically chosen aperture.
struct DriftSquare {
    Word nw, ne, se, sw;
    AX_FN Word operator[](Word j) const { return (j & 2) ? ((j & 1) ? sw : se) : ((j & 1) ? ne : nw); }
    AX_FN Word edge_root(Word e) const { return e == 1 ? ne : e == 2 ? sw : nw; }
};
#include "drift_legacy.cuh"
struct DriftWarp {
    Word mask = 0, lane = 0, leader = 0;
};
AX_FN DriftWarp drift_warp(bool aggregate, Word key, bool subdivided) {
    DriftWarp g{};
#if __CUDA_ARCH__ >= 800
    if (aggregate) {
        asm volatile("activemask.b32 %0;" : "=r"(g.mask));
        asm volatile("mov.u32 %0, %%laneid;" : "=r"(g.lane));
        if (subdivided) {
            Word matches;
            asm volatile("match.any.sync.b32 %0,%1,%2;" : "=r"(matches) : "r"(key), "r"(g.mask));
            g.mask = matches;
        }
        g.leader = Word(__builtin_ctz(g.mask));
    }
#endif
    return g;
}
AX_FN void drift_count(Count &target, Word amount, DriftWarp group) {
#if __CUDA_ARCH__ >= 800
    if (group.mask) {
        Word combined;
        asm volatile("redux.sync.add.u32 %0,%1,%2;" : "=r"(combined) : "r"(amount), "r"(group.mask));
        if (group.lane == group.leader)
            count(target, combined);
        return;
    }
#endif
    count(target, amount);
}

AX_FN drift::View drift_bed(World w, const DriftSquare &sites, Word start, Word weather, Word rock) {
    drift::View v{};
    v.inlet = start;
    v.phase = weather;
    v.rock = rock;
    for (Word e = 0; e < 4; ++e) {
        const auto &p = w.land[sites.edge_root(e)];
        v.bed.water[e] = w.land[sites[e]].water;
        v.bed.clay[e] = e & 1 ? p.south : p.east;
        v.bed.fibre[e] = e & 1 ? p.fibre_south : p.fibre_east;
    }
    return v;
}
AX_FN bool drift_bind(World w, Configuration c, const DriftSquare &sites, Word j, Word id, Word key, Word stamp,
                      drift::LegacyWatch watch, drift::Pulse &out) {
    auto &body = w.animals[id];
    const auto &g = w.dna[id];
    Word motor = w.motor[id] & 7;
    if (!c.weaving || body.energy < 3 || !body.body.limbs || !((g.grip >> (8 + motor)) & 1) || (key & 7) != (motor & 7))
        return false;
    Word edge = (j + ((g.growth >> (3 * motor)) & 3)) & 3;
    Word root = sites.edge_root(edge), other = root == sites[edge] ? sites[(edge + 1) & 3] : sites[edge];
    auto &site = w.land[root];
    auto &strand = edge & 1 ? site.fibre_south : site.fibre_east;
    Word old = strand;
    if ((g.grip >> (24 + motor)) & 1) {
        if (strand >= 65535 || (!body.fibre && !site.litter))
            return false;
        if (body.fibre)
            --body.fibre;
        else
            --site.litter;
        ++strand;
        ++out.tied;
        count(w.account->fibre_laid, 1);
    } else {
        if (!strand || body.fibre == 65535)
            return false;
        --strand;
        ++body.fibre;
        ++out.untied;
        count(w.account->fibre_lifted, 1);
    }
    practice::disturb(w.practice, c.side, root, other);
    tag_strand(w, c.side, root, other, old, strand);
    if (w.drift_marks.items && strand > old) {
        Word rank = drift::surface(c.side, root, edge & 1, w.drift_marks.salt);
        if (rank < w.drift_marks.watched) {
            w.drift_marks.items[rank] = {id,
                                         body.born,
                                         stamp,
                                         body.body.shape,
                                         body.body.limbs,
                                         body.body.eyes,
                                         body.body.pores,
                                         32 * expressed_cells(w, id, g.brain, c.tiles)};
            count(watch.counts[root & 127].placed, 1);
        }
    }
    --body.energy;
    ++body.worked;
    ++out.heat;
    return true;
}
#include "inlet_history.cuh"
template <bool Trace, bool Details = true, bool Storage = false, bool Winding = false>
AX_FN void drift_pulse(World w, Configuration c, drift::Law law, Word tick, Word tile, drift::Totals *books,
                       drift::Sampling sampling, drift::LegacyWatch inheritance, drift::Gaze gaze,
                       drift::Capture capture) {
    Word side = c.side, span = side / 2, mask = side - 1;
    Word x = (2 * (tile % span) + ((tick >> 3) & 1)) & mask, y = (2 * (tile / span) + ((tick >> 4) & 1)) & mask;
    Word xx = (x + 1) & mask, yy = (y + 1) & mask;
    DriftSquare sites{y * side + x, y * side + xx, yy * side + xx, yy * side + x};
    Word climate = mix(c.seed ^ mix((x >> 3) + 19391 * (y >> 3))),
         key = mix(c.seed ^ mix(tile + tick * 15773u) ^ 0x765aa38du);
    Word phase = (climate >> 13) & 3;
    if (law.ports & drift::tide)
        phase = (phase + (tick >> law.tide_shift)) & 3;
    drift::View view{};
    drift::Sill rim{};
    Word pressure = 0, held_stock = 0;
    if constexpr (Storage) {
        view = drift_bed(w, sites, 0, phase, climate);
        rim = drift::highest_sill(view, law.ports & drift::fibres);
        auto &lip = w.land[sites[rim.site]];
        held_stock = lip.litter ? lip.litter : lip.fruit;
        pressure = drift::head(view, law, tick, rim.site, held_stock);
        if (lip.water && held_stock) {
            auto &ledger = sampling.spills[tile & 127];
            auto bare = drift::highest_sill(view, false);
            count(ledger.inspected, 1);
            count(ledger.retained, pressure < rim.height);
            count(ledger.moved_rim, bare.site != rim.site);
            count(ledger.held_by_fibres, pressure < rim.height && (bare.site != rim.site || pressure >= bare.height));
        }
    }
    Word source = key & 3, wet = 0;
    for (Word j = 0; j < 4; ++j) {
        source = (Word(key + j)) & 3;
        if constexpr (Storage)
            if (source == rim.site && pressure < rim.height)
                continue;
        const auto &s = w.land[sites[source]];
        if (s.water && (s.litter || s.fruit)) {
            wet = 1;
            break;
        }
    }
    if (!wet)
        return;
    if constexpr (Storage)
        view.inlet = source;
    else
        view = drift_bed(w, sites, source, phase, climate);
    bool seeps = law.ports & drift::seepage;
    Word dry = drift::connections(view, false, seeps),
         links = (law.ports & drift::fibres) ? drift::connections(view, true, seeps) : dry;
    drift::Path path{links};
    if constexpr (Winding)
        path = drift::woven_path(view, links, law.ports & drift::fibres, seeps);
    Word first_outlet = drift::outlet(source, links), sink = first_outlet, reverse = first_outlet;
    if constexpr (Winding) {
        sink = path.run(source);
        reverse = path.run(source, true);
    }
    Word other = drift::outlet(source, dry);
    auto &bed = w.land[sites[source]], &mouth = w.land[sites[sink]];
    auto &stock = bed.litter ? bed.litter : bed.fruit;
    Word amount = cut(law.load, 1 + (bed.water >> 2));
    if constexpr (Storage)
        amount = cut(law.load, source == rim.site ? 1 + 2 * (pressure - rim.height) + cut(Word(stock) >> 2, 10u)
                                                  : 1u + Word(bed.water >= 8));
    amount = cut(amount, stock);
    bool same = &stock == &mouth.litter;
    if (!same)
        amount = cut(amount, 65535u - mouth.litter);
    if (!amount)
        return;
    auto &book = books[side >= 128 ? tile / 128 : 0];
    w.account += side >= 128 ? tile / 128 : 0;
    drift::Scene *sample = nullptr;
    if constexpr (Trace) {
        Word rank = (tile * 16127u + mix(c.seed)) & (side * side / 4 - 1);
        if (tick >= capture.start && tick - capture.start < capture.ticks && rank < capture.tiles) {
            sample = capture.records + Count(tick - capture.start) * capture.tiles + rank;
            sample->view = view;
            sample->tick = tick + 1;
            sample->tile = tile;
            sample->origin = sites[0];
            sample->links = links;
            sample->unbound = dry;
            sample->sink = sink;
            sample->amount = amount;
            sample->source_stock = stock;
            sample->dest_litter = mouth.litter;
            sample->same_stock = same;
            sample->rim = rim.site;
            sample->height = rim.height;
            sample->pressure = pressure;
            sample->held_stock = held_stock;
        }
    }
    DriftSurface marked{};
    if constexpr (Details)
        if (w.drift_marks.items)
            marked = find_drift_surface(w, side, sites, key >> 22, tick + 1);
    Word aims = 0, capacities = 0, meals = 0;
    Word rank = (tile * 16127u + mix(c.seed)) & (side * side / 4 - 1), candidate = ~0u, actors = 0;
    Word base_colors = 0, dye = bed.litter ? bed.spore & 3 : (bed.root >> 16) & 3;
    if constexpr (Details)
        if ((law.ports & drift::sight) || Winding)
            for (Word j = 0; j < 4; ++j)
                base_colors |= Word(w.braids.floats[sites[j]] & 3) << (2 * j);
    if constexpr (Winding && !Details)
        for (Word j = 0; j < 4; ++j)
            base_colors |= Word(w.braids.floats[sites[j]] & 3) << (2 * j);
    if constexpr (Winding && Trace)
        if (sample) {
            sample->upper = path.upper;
            sample->turns = path.turns;
            sample->laps = path.length;
            sample->resting = base_colors;
            sample->dye = dye;
        }
    Word reversed_food = 0, serial_reaches = 0, serial_right = 0, single_right = 0, assured = 0;
    bool rafting = law.ports & drift::rafting, rafts_moved = false;
    Word raft_load = 0;
    if (rafting && source != sink) {
        auto from = w.braids.floats[sites[source]], to = w.braids.floats[sites[sink]];
        Word cargo = braid::mass(from) + braid::mass(to);
        raft_load = cargo;
        Word drive = amount + cut(bed.water, 48u) / 4;
        rafts_moved = drive >= 1 + (cargo >> 1);
        if (rafts_moved) {
            w.braids.floats[sites[sink]] = from | 128u;
            w.braids.floats[sites[source]] = to | 128u;
        }
        auto &book = sampling.rafts[tile & 127u];
        count(book.opportunities, 1);
        count(book.swaps, rafts_moved);
        count(book.blocked, !rafts_moved);
        if (rafts_moved) {
            count(book.carbon, cargo);
            count(book.travelled, braid::distance(from, sites[sink] % side, sites[sink] / side) >= 3);
            count(book.travelled, braid::distance(to, sites[source] % side, sites[source] / side) >= 3);
        }
    }
    Word priority = (key >> 9) & 3, eaten = 0, blank_eaten = 0, unbound_eaten = 0;
    drift::Pulse totals{};
    totals.pulses = 1;
    totals.released = amount;
    totals.crossed = sink != source;
    totals.fibre_diversions = sink != other;
    // The computation below is equivalent to simultaneous commitments with
    // priority access to finite food. Earlier hands never change later inputs.
    for (Word z = 0; z < 4; ++z) {
        Word j = (priority + z) & 3, id = w.slots[sites[j]];
        auto &a = w.animals[id];
        const auto &g = w.dna[id];
        auto &m = w.braids.minds[id];
        Word motor = w.motor[id] & 7;
        if (a.energy < 2 || !a.body.limbs || !a.body.pores || !((g.grip >> (16 + motor)) & 1))
            continue;
        Word band = motor_band(w, c, id);
        auto tissue = read_band(w, c.tiles, id, band);
        auto organ = regulation::source(g, band, tissue);
        Word row = a.body.eyes ? drift::face(m.genes.arbor >> 25, source) : ((tissue.spike >> (motor + 1)) & 3);
        Word axes = (m.genes.muscle >> 24) & 7;
        Word group = braid::drift_tag(m.lived);
        Word ports = drift::branch(law.ports, group);
        Word synapse = (law.ports & drift::tidal_banks) && a.body.eyes
                           ? retina::bank({g.pigments[0], g.pigments[1]},
                                          braid::pigment(braid::eyes_of(w, id).hues, retina::tide_lamp(phase, climate)))
                           : 0;
        Word original_hand = m.volley >> 16;
        retina::Banks banks{original_hand, tissue.gate_a, tissue.gate_b};
        Word old = banks.get(synapse), wiring = drift::wiring(m.genes, law.ports);
        Word available = drift::openings(old, row, wiring, ports), one_stage = available, displaced_options = ~0u;
        if constexpr (Winding)
            if (a.body.eyes) {
                auto eye = braid::eyes_of(w, id);
                retina::Genes genes{g.pigments[0], g.pigments[1]};
                auto now = retina::project(retina::reflected(base_colors, dye, source, source), eye);
                Word onset = retina::initial(genes, now.rest, now.onset, a.heading);
                Word light = retina::tide_lamp(phase, climate);
                Word lower = braid::pigment(eye.hues, light), upper = braid::pigment(eye.hues, (light + 2) & 3);
                auto signal = [&](Word j) { return (path.turns & (1u << j)) ? upper : lower; };
                available = retina::sequence(onset, banks, genes, wiring, ports, path.length,
                                             law.ports & drift::tidal_banks, signal, &one_stage);
                if constexpr (Details)
                    if (gaze.first.visits)
                        displaced_options = retina::sequence(onset, banks, genes, wiring, ports ^ drift::misaligned,
                                                             path.length, law.ports & drift::tidal_banks, signal);
            }
        Word draw = mix(key ^ mix(id + 631 * row + motor) ^ tissue.spike);
        Word blank = drift::choose(15, draw);
        bool reads = drift::reads(law.ports, group);
        Word command = reads ? drift::choose(available, draw) : blank;
        Word aim = drift::face(axes, command);
        Word capacity = cut(1 + nbits(a.body.pores), 65536u - a.energy);
        Word bite = aim == sink ? cut(capacity, amount - eaten) : 0u;
        if constexpr (Winding) {
            if (aim == reverse)
                reversed_food += cut(capacity, amount - reversed_food);
            if (path.length > 1) {
                ++serial_reaches;
                serial_right += aim == sink;
                single_right += drift::face(axes, reads ? drift::choose(one_stage, draw) : blank) == sink;
                assured += reads && available && !(available & (available - 1));
            }
        }
        Word solo_blank = drift::face(axes, blank) == sink ? cut(capacity, amount - eaten) : 0u;
        PriorInlets experience{};
        if constexpr (Details)
            if (gaze.first.visits) {
                Word local = drift::choose((~old >> (4 * row)) & 15, draw);
                Word local_food = drift::face(axes, local) == sink ? cut(capacity, amount - eaten) : 0u;
                experience =
                    first_inlet(w, c, gaze, sampling, id, row, sites[0], Winding ? path.shape() : links,
                                reads ? available : 15u, old, wiring, reads ? ports : 0u, draw, axes, command, sink,
                                bite, local_food, organ.program, Storage && source == rim.site, displaced_options);
            }
        if constexpr (Details)
            if (marked.maker.stamp) {
                aims |= aim << (2 * j);
                capacities |= capacity << (5 * j);
                meals |= bite << (5 * j);
            }
        if (drift::face(axes, blank) == sink)
            blank_eaten += cut(capacity, amount - blank_eaten);
        if (aim == other)
            unbound_eaten += cut(capacity, amount - unbound_eaten);
        --a.energy;
        ++a.worked;
        ++totals.heat;
        ++totals.reached;
        totals.learned_choices += command != blank;
        eaten += bite;
        a.energy += flood::Half(bite);
        a.meals += bite;
        a.diet[1] += bite;
        totals.hit += bite != 0;
        bool committed = available && !(available & (available - 1));
        totals.committed += committed;
        totals.confirmed += committed && bite;
        if (tick + 1 - a.born <= 32) {
            ++totals.young_reaches;
            totals.young_meals += bite;
        } else {
            ++totals.old_reaches;
            totals.old_meals += bite;
        }
        bool learns = c.plastic && (law.ports & drift::plastic) && (m.genes.arbor & (1u << 12));
        Word after = old;
        if (learns) {
            after = drift::strength(old, row, command, bite != 0, m.genes.plastic, organ, law.ports & drift::feedback,
                                    law.ports & drift::regrowth, available);
            banks.put(synapse, after);
            if (!synapse)
                m.volley = (m.volley & 65535u) | (after << 16);
            totals.contact_edits += nbits(old ^ after);
            totals.somatic_edits += nbits(tissue.spike ^ organ.cells.spike);
            if (tissue.spike != organ.cells.spike || (synapse && after != old)) {
                auto t = tissue;
                t.spike = organ.cells.spike;
                t.gate_a = banks.a;
                t.gate_b = banks.b;
                assign_band(w, c.tiles, id, band, t);
            }
        }
        if constexpr (Details)
            if ((law.ports & drift::sight) && a.body.eyes) {
                auto eyes = braid::eyes_of(w, id);
                Word midpoint = !rafting && (law.ports & (drift::lateral | drift::seepage | drift::storage))
                                    ? drift::horizontal(source, links)
                                    : 4u;
                auto scene = drift::sighting(base_colors, dye, source, sink, reads ? available : 15, axes, tick + 1,
                                             eyes, rafting, rafts_moved, midpoint);
                w.braids.episodes[id] = scene;
                actors |= 1u << j;
                auto observed = [&]() {
                    return drift::Exhibit{id,
                                          a.born,
                                          tick + 1,
                                          tile,
                                          source,
                                          sink,
                                          aim,
                                          row,
                                          old,
                                          axes,
                                          base_colors,
                                          dye,
                                          eyes.places,
                                          eyes.hues,
                                          Word(rafting),
                                          Word(rafts_moved),
                                          raft_load,
                                          bed.water,
                                          scene,
                                          wiring,
                                          reads ? available : 15u,
                                          drift::choose((~old >> (4 * row)) & 15, draw),
                                          experience.prior,
                                          experience.nearby,
                                          experience.reaches,
                                          midpoint};
                };
                if (gaze.diary.notes && id == gaze.diary.slot && a.born == gaze.diary.born &&
                    tick >= gaze.diary.start && tick - gaze.diary.start < gaze.diary.ticks)
                    gaze.diary.notes[tick - gaze.diary.start] = {
                        observed(), after, command, blank, amount, bite, reads ? available : 15};

                if (gaze.choice && rank < gaze.tiles && id < candidate && reads && committed &&
                    (gaze.kind >= 3 ||
                     braid::picture(scene, braid::beginning) != braid::picture(scene, braid::result))) {
                    Word old_ports = gaze.kind == 4 ? experience.nearby : experience.prior;
                    bool untried = experience.observed && old_ports == (15u ^ (1u << row)) &&
                                   command != drift::choose((~old >> (4 * row)) & 15, draw);
                    bool matches = gaze.kind >= 3 ? untried : (gaze.kind == 1 ? aim != sink : aim == sink);
                    bool public_conflict =
                        braid::picture(scene, braid::result) != braid::picture(scene, braid::intended);
                    bool audible = !gaze.selection || gaze.selection == 3 ||
                                   ((m.genes.arbor & (3u << 14)) == (3u << 14) &&
                                    (gaze.eligible & (1u << ((m.genes.plastic >> 8) & 15))));
                    bool related = gaze.selection < 2 || drift::matches(gaze.target, organ.program, m.genes);
                    if (matches && (gaze.kind != 1 || public_conflict) && audible && related) {
                        candidate = id;
                        gaze.events[rank] = observed();
                    }
                }
            }
        if (sampling.bins) {
            Word since = tick & ((1u << law.tide_shift) - 1);
            Word quarter = (since * 4) >> law.tide_shift, shard = side >= 128 ? tile / 1024 : 0;
            auto record = [&](Word which) {
                Word target = (shard * drift::populations + which) * drift::phases + quarter;
                auto &b = sampling.bins[target];
                auto warp = drift_warp(sampling.aggregate&2u, target, true);
                drift_count(b.reached, 1, warp);
                drift_count(b.eaten, bite, warp);
                drift_count(b.without, solo_blank, warp);
                drift_count(b.changed, command != blank, warp);
                drift_count(b.hits, bite != 0, warp);
                drift_count(b.committed, committed, warp);
                drift_count(b.judged, committed && bite, warp);
                drift_count(b.edits, nbits(old ^ after), warp);
            };
            record(group);
            if (sampling.has_donor && drift::matches(sampling.donor, organ.program, m.genes))
                record(3);
        }
        if constexpr (Trace)
            if (sample)
                sample->reach[j] = {id,
                                    a.born,
                                    row,
                                    axes,
                                    draw,
                                    old,
                                    command,
                                    blank,
                                    capacity,
                                    bite,
                                    after,
                                    tissue.spike,
                                    organ.cells.background,
                                    organ.program.first,
                                    organ.program.second,
                                    m.genes.plastic & 15u,
                                    1,
                                    Word(learns),
                                    organ.cells.spike,
                                    group,
                                    Word(reads),
                                    wiring,
                                    available,
                                    Word(Winding && a.body.eyes),
                                    g.pigments[0],
                                    g.pigments[1],
                                    Word(braid::eyes_of(w, id).places) | (Word(braid::eyes_of(w, id).hues) << 8),
                                    a.heading,
                                    original_hand,
                                    tissue.gate_a,
                                    tissue.gate_b};
    }
    if constexpr (Details)
        if (law.ports & drift::audience)
            for (Word j = 0; j < 4; ++j)
                if (!(actors & (1u << j))) {
                    Word id = w.slots[sites[j]];
                    auto &a = w.animals[id];
                    if (a.energy && a.body.eyes)
                        w.braids.episodes[id] = drift::sighting(base_colors, dye, source, sink, 0, 0, tick + 1,
                                                                braid::eyes_of(w, id), rafting, rafts_moved);
                }
    stock -= flood::Half(amount);
    mouth.litter += flood::Half(amount - eaten);
    totals.eaten = eaten;
    totals.settled = amount - eaten;
    totals.blank_eaten = blank_eaten;
    totals.unbound_eaten = unbound_eaten;
    totals.gained = eaten > blank_eaten ? eaten - blank_eaten : 0;
    totals.lost = blank_eaten > eaten ? blank_eaten - eaten : 0;
    totals.fibre_gain = eaten > unbound_eaten ? eaten - unbound_eaten : 0;
    totals.fibre_loss = unbound_eaten > eaten ? unbound_eaten - eaten : 0;
    totals.mobile_deposit = sink != source ? amount - eaten : 0;
    if constexpr (Trace)
        if (sample) {
            sample->priority = priority;
            sample->after_stock = stock;
            sample->after_litter = mouth.litter;
            sample->eaten = eaten;
            sample->blank_eaten = blank_eaten;
            sample->unbound_eaten = unbound_eaten;
        }
    if constexpr (Details)
        if (marked.maker.stamp)
            inspect_drift_surface(w, side, c.tiles, sites, tile, tick + 1, law, view, marked, inheritance, sink, amount,
                                  priority, aims, capacities, meals, eaten);
    if constexpr (Storage) {
        auto &ledger = sampling.spills[tile & 127];
        bool flood = source == rim.site;
        count(flood ? ledger.floods : ledger.trickles, 1);
        count(flood ? ledger.flood_stock : ledger.trickle_stock, amount);
        count(flood ? ledger.flood_meals : ledger.trickle_meals, eaten);
    }
    if (law.ports & drift::building) {
        Word j = (key >> 15) & 3;
        if (drift_bind(w, c, sites, j, w.slots[sites[j]], key >> 17, tick + 1, inheritance, totals)) {
            auto amended = drift_bed(w, sites, source, phase, climate);
            Word updated = drift::connections(amended, law.ports & drift::fibres, seeps);
            totals.changed_routes = updated != links;
            if constexpr (Winding) {
                auto next = drift::woven_path(amended, updated, law.ports & drift::fibres, seeps);
                bool altered = path.length != next.length || path.turns != next.turns;
                bool map = false;
                for (Word j = 0; j < 4; ++j)
                    map |= next.run(j) != path.run(j);
                auto &routes = sampling.courses[tile & 127];
                count(routes.altered, altered);
                count(routes.altered_map, map);
                totals.changed_routes = map;
            }
        }
    }
    if constexpr (Winding) {
        auto &r = sampling.courses[(tile >> 5) & 127u];
        auto group = drift_warp(true, (tile >> 5) & 127u, law.stride_shift != 0);
        drift_count(r.pulses, 1, group);
        drift_count(r.serial, path.length > 1, group);
        drift_count(r.stages, path.length, group);
        drift_count(r.order_changed, sink != reverse, group);
        drift_count(r.beyond_first, sink != first_outlet, group);
        drift_count(r.material, path.length > 1 ? amount : 0, group);
        drift_count(r.eaten, path.length > 1 ? eaten : 0, group);
        drift_count(r.reversed_food, path.length > 1 ? reversed_food : 0, group);
        drift_count(r.reaches, serial_reaches, group);
        drift_count(r.right, serial_right, group);
        drift_count(r.single_right, single_right, group);
        drift_count(r.committed, assured, group);
    }
    auto warp = drift_warp(sampling.aggregate&1u, tile / 128, side >= 128 && law.stride_shift == 3);
    drift_count(w.account->harvest, eaten, warp);
    drift_count(w.account->digested, eaten, warp);
    drift_count(w.account->heat, totals.heat, warp);
    // The ledger is separate from physics. Each named counter is an
    // unsigned 64-bit reduction into a spatial shard.
#define DRIFT_ADD(member) drift_count(book.member, totals.member, warp)
    DRIFT_ADD(pulses);
    DRIFT_ADD(released);
    DRIFT_ADD(eaten);
    DRIFT_ADD(settled);
    DRIFT_ADD(reached);
    DRIFT_ADD(hit);
    DRIFT_ADD(heat);
    DRIFT_ADD(committed);
    DRIFT_ADD(confirmed);
    DRIFT_ADD(contact_edits);
    DRIFT_ADD(somatic_edits);
    DRIFT_ADD(learned_choices);
    DRIFT_ADD(blank_eaten);
    DRIFT_ADD(gained);
    DRIFT_ADD(lost);
    DRIFT_ADD(unbound_eaten);
    DRIFT_ADD(fibre_diversions);
    DRIFT_ADD(fibre_gain);
    DRIFT_ADD(fibre_loss);
    DRIFT_ADD(tied);
    DRIFT_ADD(untied);
    DRIFT_ADD(changed_routes);
    DRIFT_ADD(crossed);
    DRIFT_ADD(mobile_deposit);
    DRIFT_ADD(young_reaches);
    DRIFT_ADD(young_meals);
    DRIFT_ADD(old_reaches);
    DRIFT_ADD(old_meals);
#undef DRIFT_ADD
}
} // namespace
KERNEL void flood_drift(World w, Configuration c, const Word *clock, Word offset, drift::Law law, drift::Totals *totals,
                        drift::Sampling sampling, drift::LegacyWatch inheritance, drift::Gaze gaze) {
    Word tick = *clock + offset, mask = (1u << law.stride_shift) - 1;
    Word tile = (tid() << law.stride_shift) + (tick & mask);
    if (tile < c.side * c.side / 4)
        drift_pulse<false>(w, c, law, tick, tile, totals, sampling, inheritance, gaze, {});
}
KERNEL void flood_drift_core(World w, Configuration c, const Word *clock, Word offset, drift::Law law,
                             drift::Totals *totals, drift::Sampling sampling, drift::LegacyWatch inheritance,
                             drift::Gaze gaze) {
    Word tick = *clock + offset, mask = (1u << law.stride_shift) - 1,
         tile = (tid() << law.stride_shift) + (tick & mask);
    if (tile < c.side * c.side / 4)
        drift_pulse<false, false>(w, c, law, tick, tile, totals, sampling, inheritance, gaze, {});
}
KERNEL void flood_drift_record(World w, Configuration c, const Word *clock, Word offset, drift::Law law,
                               drift::Totals *totals, drift::Sampling sampling, drift::LegacyWatch inheritance,
                               drift::Gaze gaze, drift::Capture capture) {
    Word tick = *clock + offset, mask = (1u << law.stride_shift) - 1;
    Word tile = (tid() << law.stride_shift) + (tick & mask);
    if (tile < c.side * c.side / 4)
        drift_pulse<true>(w, c, law, tick, tile, totals, sampling, inheritance, gaze, capture);
}
KERNEL void flood_sill(World w, Configuration c, const Word *clock, Word offset, drift::Law law, drift::Totals *totals,
                       drift::Sampling sampling, drift::LegacyWatch inheritance, drift::Gaze gaze) {
    Word tick = *clock + offset, tile = (tid() << law.stride_shift) + (tick & ((1u << law.stride_shift) - 1));
    if (tile < c.side * c.side / 4)
        drift_pulse<false, true, true>(w, c, law, tick, tile, totals, sampling, inheritance, gaze, {});
}
KERNEL void flood_sill_core(World w, Configuration c, const Word *clock, Word offset, drift::Law law,
                            drift::Totals *totals, drift::Sampling sampling, drift::LegacyWatch inheritance,
                            drift::Gaze gaze) {
    Word tick = *clock + offset, tile = (tid() << law.stride_shift) + (tick & ((1u << law.stride_shift) - 1));
    if (tile < c.side * c.side / 4)
        drift_pulse<false, false, true>(w, c, law, tick, tile, totals, sampling, inheritance, gaze, {});
}
KERNEL void flood_sill_record(World w, Configuration c, const Word *clock, Word offset, drift::Law law,
                              drift::Totals *totals, drift::Sampling sampling, drift::LegacyWatch inheritance,
                              drift::Gaze gaze, drift::Capture capture) {
    Word tick = *clock + offset, tile = (tid() << law.stride_shift) + (tick & ((1u << law.stride_shift) - 1));
    if (tile < c.side * c.side / 4)
        drift_pulse<true, true, true>(w, c, law, tick, tile, totals, sampling, inheritance, gaze, capture);
}

KERNEL void flood_woven(World w, Configuration c, const Word *clock, Word offset, drift::Law law, drift::Totals *totals,
                        drift::Sampling sampling, drift::LegacyWatch inheritance, drift::Gaze gaze) {
    Word tick = *clock + offset, mask = (1u << law.stride_shift) - 1;
    Word tile = (tid() << law.stride_shift) + (tick & mask);
    if (tile < c.side * c.side / 4)
        drift_pulse<false, true, false, true>(w, c, law, tick, tile, totals, sampling, inheritance, gaze, {});
}
KERNEL void flood_woven_core(World w, Configuration c, const Word *clock, Word offset, drift::Law law,
                             drift::Totals *totals, drift::Sampling sampling, drift::LegacyWatch inheritance,
                             drift::Gaze gaze) {
    Word tick = *clock + offset, mask = (1u << law.stride_shift) - 1,
         tile = (tid() << law.stride_shift) + (tick & mask);
    if (tile < c.side * c.side / 4)
        drift_pulse<false, false, false, true>(w, c, law, tick, tile, totals, sampling, inheritance, gaze, {});
}
KERNEL void flood_woven_record(World w, Configuration c, const Word *clock, Word offset, drift::Law law,
                               drift::Totals *totals, drift::Sampling sampling, drift::LegacyWatch inheritance,
                               drift::Gaze gaze, drift::Capture capture) {
    Word tick = *clock + offset, mask = (1u << law.stride_shift) - 1;
    Word tile = (tid() << law.stride_shift) + (tick & mask);
    if (tile < c.side * c.side / 4)
        drift_pulse<true, true, false, true>(w, c, law, tick, tile, totals, sampling, inheritance, gaze, capture);
}
KERNEL void flood_sill_woven(World w, Configuration c, const Word *clock, Word offset, drift::Law law,
                             drift::Totals *totals, drift::Sampling sampling, drift::LegacyWatch inheritance,
                             drift::Gaze gaze) {
    Word tick = *clock + offset, tile = (tid() << law.stride_shift) + (tick & ((1u << law.stride_shift) - 1));
    if (tile < c.side * c.side / 4)
        drift_pulse<false, true, true, true>(w, c, law, tick, tile, totals, sampling, inheritance, gaze, {});
}
KERNEL void flood_sill_woven_core(World w, Configuration c, const Word *clock, Word offset, drift::Law law,
                                  drift::Totals *totals, drift::Sampling sampling, drift::LegacyWatch inheritance,
                                  drift::Gaze gaze) {
    Word tick = *clock + offset, tile = (tid() << law.stride_shift) + (tick & ((1u << law.stride_shift) - 1));
    if (tile < c.side * c.side / 4)
        drift_pulse<false, false, true, true>(w, c, law, tick, tile, totals, sampling, inheritance, gaze, {});
}
KERNEL void flood_sill_woven_record(World w, Configuration c, const Word *clock, Word offset, drift::Law law,
                                    drift::Totals *totals, drift::Sampling sampling, drift::LegacyWatch inheritance,
                                    drift::Gaze gaze, drift::Capture capture) {
    Word tick = *clock + offset, tile = (tid() << law.stride_shift) + (tick & ((1u << law.stride_shift) - 1));
    if (tile < c.side * c.side / 4)
        drift_pulse<true, true, true, true>(w, c, law, tick, tile, totals, sampling, inheritance, gaze, capture);
}

KERNEL void drift_founders(World w, Word n, Word seed, Word marking, Word stock, braid::Genes genes) {
    Word id = tid();
    if (id >= n)
        return;
    auto &m = w.braids.minds[id];
    if (stock && w.animals[id].energy)
        m = {genes};
    if (marking) {
        Word tag = 0;
        if (w.animals[id].energy)
            tag = 1 + ((id ^ mix((id >> 1) + seed)) & 1);
        m.lived = (m.lived & ~braid::lineage_bits) | (tag << 14);
    }
}
KERNEL void drift_population(World w, Configuration c, Word start, drift::Headcount *census) {
    Word i = tid();
    if (i >= c.side * c.side)
        return;
    const auto &a = w.animals[i];
    if (!a.energy)
        return;
    const auto &m = w.braids.minds[i];
    Word group = braid::drift_tag(m.lived), shard = c.side >= 128 ? i / 256 : 0;
    auto &q = census[3 * shard + group];
    count(q.alive, 1);
    count(q.young, a.born > start);
    count(q.energy, a.energy);
    count(q.bits, nbits(m.volley >> 16));
    count(q.cells, 32 * expressed_cells(w, i, w.dna[i].brain, c.tiles));
}
KERNEL void drift_germlines(World w, Configuration c, const drift::Germline *left, Word na,
                            const drift::Germline *right, Word nb, Word swap, Word loci, Word seed) {
    Word id = tid();
    if (id >= c.side * c.side || !w.animals[id].energy)
        return;
    auto &hand = w.braids.minds[id];
    Word group = braid::drift_tag(hand.lived);
    if (!group)
        return;
    bool b = (group == 2) ^ bool(swap);
    Word draw = mix(seed + mix((id >> 1) + 37453u));
    const auto donor = b ? right[draw % nb] : left[draw % na];
    if (loci & 1) {
        auto &g = w.dna[id];
        Word band = motor_band(w, c, id);
        g.module[regulation::module_index(g, band)] = {donor.program.first, donor.program.second};
    }
    if (loci & 2)
        hand.genes.plastic = (hand.genes.plastic & ~15u) | donor.law;
    if (loci & 4) {
        drift::wire(hand.genes, donor.wiring, true);
        hand.genes.arbor = (hand.genes.arbor & ~(1u << 12)) | (donor.active << 12);
    }
}

KERNEL void drift_witness_check(World w, const Word *clock, Word offset, drift::Gaze gaze) {
    Word rank = tid();
    if (rank >= gaze.tiles)
        return;
    const auto &e = gaze.events[rank];
    if (e.stamp != *clock + offset + 1)
        return;
    const auto &a = w.animals[e.id];
    const auto &seen = w.braids.episodes[e.id];
    if (a.energy && a.born == e.born && seen.stamp == e.scene.stamp && seen.images == e.scene.images &&
        seen.known == e.scene.known)
        asm volatile("red.global.min.u32 [%0],%1;" ::"l"(gaze.choice), "r"(rank) : "memory");
}
// This isolated tissue assay uses the same hydraulic crossing law and local
// writes as the watershed. The selection and confirmation panels have disjoint
// climate seeds. Each seasonal change occurs without an advance cue.
KERNEL void drift_food_test(const drift::Germline *alleles, Word count, Word seed, Word lives, Word ports,
                            drift::Diet *out) {
    Word i = tid();
    if (i >= count)
        return;
    const auto a = alleles[i];
    drift::Diet total{};
    for (Word life = 0; life < lives; ++life) {
        Word k = mix(seed + life * 0x9e3779b9u);
        drift::View view{};
        view.rock = mix(k + 751);
        view.phase = (k >> 7) & 3;
        for (Word j = 0; j < 4; ++j) {
            Word terrain = mix(k + j * 503 + 31);
            view.bed.water[j] = 9 + (terrain & 31);
            view.bed.clay[j] = (terrain >> 7) & 3;
            view.bed.fibre[j] = ((terrain >> 15) & 1) ? 0 : (terrain >> 17) & 3;
        }
        Word route = drift::connections(view, true, ports & drift::seepage);
        view.phase ^= 2;
        Word altered = drift::connections(view, true, ports & drift::seepage);
        regulation::Source source{a.program, {0, 0}, 0};
        Word excluded = 0;
        for (Word pulse = 0; pulse < 96; ++pulse) {
            Word wave = mix(k + pulse * 0x7f4a7c15u), inlet = (wave >> 4) & 3, draw = mix(wave ^ 0xadc9b37u),
                 phase = pulse / 32;
            Word sink = drift::outlet(inlet, phase == 1 ? altered : route), off = drift::choose(15, draw);
            Word options = drift::openings(excluded, inlet, a.wiring, ports);
            Word chosen = drift::choose(options, draw);
            bool hit = chosen == sink, distinct = phase && drift::outlet(inlet, route) != drift::outlet(inlet, altered);
            total.food += hit;
            total.blank += off == sink;
            total.phase[phase] += hit;
            total.changed += distinct;
            total.changed_food += distinct && hit;
            if (a.active)
                excluded = drift::strength(excluded, inlet, chosen, hit, a.law, source, true, ports & drift::regrowth,
                                           options);
        }
    }
    out[i] = total;
}
