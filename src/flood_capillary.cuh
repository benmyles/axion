#include "axion/capillary_field.hpp"
#include "axion/material_hands.hpp"
#include "chemical_wake.cuh"

AX_FN Word capillary_grain(const Silt &s, Word threshold) {
    return Word(s.fibre_east != 0) | (Word(s.water >= threshold && !s.south) << 1) | (Word(s.east != 0) << 2) |
           (Word(s.fibre_south != 0) << 3);
}
AX_FN Half &capillary_edge(Silt &s, Word kind) {
    return kind == 0 ? s.fibre_east : kind == 1 ? s.south : kind == 2 ? s.east : s.fibre_south;
}
AX_FN Half &capillary_cargo(Colony &body, Word kind) { return (kind == 0 || kind == 3) ? body.fibre : body.load; }
AX_FN Half *repair_stock(Silt &s, Colony &a, Word k, bool recycling, Word &donation) {
    auto &cargo = capillary_cargo(a, k);
    if (capillary_edge(s, k) || cargo || !recycling)
        return &cargo;
    if (k == 1 || k == 2) {
        donation = 1;
        return &s.grains;
    }
    donation = 2;
    return &capillary_edge(s, 3 - k);
}
AX_FN Word candidate_edge(Word &row, Silt &s, Colony &a, Word k, Word threshold, bool recycling) {
    Word source = 0;
    auto edge = capillary_edge(s, k), stock = *repair_stock(s, a, k, recycling, source);
    bool lift = edge != 0;
    if (lift ? stock == 65535 : !stock)
        return 0;
    bool next = lift ? edge > 1 : true;
    if (k == 1)
        next = s.water >= threshold && !next;
    row = (row & ~(1u << (2 + k))) | (Word(next) << (2 + k));
    if (source == 2 && stock == 1)
        row &= ~(1u << (2 + (3 - k)));
    return lift ? 2u : 1u;
}
struct MaterialExchange {
    Word edge0 = 0, edge1 = 0, cargo0 = 0, cargo1 = 0, donation = 0;
};
AX_FN MaterialExchange exchange_capillary_piece(World w, Configuration c, Word i, Word x, Word y, Word kind, Silt &site,
                                                Colony &animal, bool sorting) {
    MaterialExchange r{};
    auto &part = capillary_edge(site, kind);
    auto &cargo = *repair_stock(site, animal, kind, sorting, r.donation);
    r.edge0 = part;
    r.cargo0 = cargo;
    if (part) {
        --part;
        ++cargo;

    } else {
        --cargo;
        ++part;
    }
    r.edge1 = part;
    r.cargo1 = cargo;
    if (kind == 0 || kind == 3) {
        auto disturb = [&](Word axis, Word before, Word after) {
            Word other = axis == 0 ? y * c.side + ((x + 1) & (c.side - 1)) : ((y + 1) & (c.side - 1)) * c.side + x;
            practice::disturb(w.practice, c.side, i, other);
            tag_strand(w, c.side, i, other, before, after);
        };
        Word axis = kind == 3;
        disturb(axis, r.edge0, r.edge1);
        if (r.donation == 2)
            disturb(axis ^ 1u, r.cargo0, r.cargo1);
    }
    return r;
}
AX_FN capillary::SignalMemory watershed_forager(World w, Configuration c, Word slot, Word band) {
    auto &g = w.dna[slot];
    auto &life = w.animals[slot];
    auto &nerve = w.braids.minds[slot];
    auto cortex = regulation::source(g, band, read_band(w, c.tiles, slot, band));
    capillary::Body body{{nerve.genes.plastic >> 8, nerve.genes.arbor, cortex.program, 1, g.hearing.expression},
                         cortex.cells,
                         g.pigments[2],
                         g.grip,
                         Word(bool(life.body.eyes)),
                         Word(bool(life.body.limbs)),
                         Word(bool(life.body.pores))};
    return {w.chemical_contacts[slot], body};
}
AX_FN void keep_foraging_spikes(World w, Configuration c, Word slot, Word band, const capillary::SignalMemory &m) {
    auto soma = read_band(w, c.tiles, slot, band);
    if (soma.spike != m.body.cells.spike) {
        soma.spike = m.body.cells.spike;
        assign_band(w, c.tiles, slot, band, soma);
    }
}
struct PressureHead {
    Word a, b, tide, threshold;
    bool spring;
};
AX_FN PressureHead upstream_head(Configuration c, capillary::Device deck, Word index, Word i, Word x, Word y,
                                 Word words, const Silt &site) {
    const Word *prior = deck.waves + (index & 1) * words;
    Word west = y * c.side + ((x - 1) & (c.side - 1)), north = ((y - 1) & (c.side - 1)) * c.side + x;
    Word a = (prior[west >> 5] >> (west & 31)) & 1u, b = (prior[north >> 5] >> (north & 31)) & 1u;

    Word patch = mix(c.seed ^ mix((x >> 2) + 11239u * (y >> 2)));
    Word tide = (index + (patch >> 15)) & 31, threshold = 2 + ((patch >> 21) & 15) + 2 * (tide < 16 ? tide : 31 - tide);
    if (!tide) {
        a = 0;
        b = 0;
    }
    bool spring = site.water >= threshold && (((index + ((patch >> 7) & 31)) & 15) == 0) &&
                  ((mix(c.seed + i * 1301) >> 6) & 7) == 0;
    a |= spring;
    return {a, b, tide, threshold, spring};
}

#include "field_hands.cuh"
template <bool record, bool adaptive = false, bool compare = false, bool ports = false, bool listen = false>
AX_FN void capillary_tick(World w, Configuration c, const Word *clock, Word offset, capillary::Device deck) {
    Word tick = *clock + offset, i = tid(), n = c.side * c.side;
    if ((tick & ((1u << deck.law.shift) - 1)) || i >= n)
        return;
    Word index = tick >> deck.law.shift, x = i & (c.side - 1), y = i / c.side, words = (n + 31) / 32;
    Word *next = deck.waves + ((index + 1) & 1) * words;
    auto &site = w.land[i];
    auto [a, b, tide, threshold, spring] = upstream_head(c, deck, index, i, x, y, words, site);
    Word row = a | (b << 1) | (capillary_grain(site, threshold) << 2), early = row;
    Word geology = capillary::rock(deck.law, c.seed, x, y), slot = w.slots[i],
         coin = mix(c.seed ^ mix(i + index * 13441));
    auto &animal = w.animals[slot];
    auto &g = w.dna[slot];
    Word *pad = deck.waves + 2 * words;
    Word old_dye = w.braids.floats[i] & 3;
    Word paint = ((pad[i >> 5] >> (i & 31)) & 1u) | (((pad[words + (i >> 5)] >> (i & 31)) & 1u) << 1);
    const bool reactive = bool(deck.law.mode & capillary::catalytic_surface);
    Word stone = reactive ? capillary::conditioned_mineral(geology, paint, old_dye) : geology;
    bool sorting = deck.law.mode & 8;
    Word behavior = deck.law.mode & 7;
    Word appetite = (g.stomach >> (2 * ((animal.action + animal.heading) & 7))) & 3;
    bool valued = !sorting || paint == appetite || old_dye == appetite;
    Word want = 1;
    if (sorting)
        want = (paint == appetite) != (old_dye == appetite) ? Word(paint == appetite) : g.grip >> 31;
    bool awake = animal.energy > 4 && (g.pigments[0] & (1u << (16 + (animal.action & 7)))) && ((coin >> 11) & 3) == 0;
    Word hand = 0, placed = 0, lifted = 0, prediction = capillary::absent, cast = 0, bite = 0, cost = 0, edits = 0,
         spikes = 0, pair = 0, inferred = 0;
    Word stock = site.litter, energy = animal.energy, learned_action = 0;
    Word cargo0 = 0, cargo1 = 0, edge0 = 0, edge1 = 0, donation = 0;
    capillary::Tissue original{};
    bool contextual = adaptive && behavior != 4;
    capillary::Revision compartment{};
    capillary::History old_history{};
    Word memory_gate = 0;
    Word tested = 0, written = 0;
    Word port = 0, light = 0, movement = 0, pump = 0, basal_row = row, weight_before = 0, weight_after = 0,
         food_edits = 0, band = 0, cold_choice = 0, cold_cast = 0, cold_food = 0, other_light_move = 0, other_bare = 0;
    if constexpr (ports) {
        Word activity = animal.action & 7;
        if (awake && animal.body.eyes && animal.body.limbs && animal.body.pores && ((g.grip >> activity) & 1)) {
            // Expression can change with both the genome and the current motor state.
            port = 1 + ((g.hearing.expression >> (8 + activity)) & 1);
            light = port == 1 ? stock >= (1 + ((g.stomach >> 12) & 15)) : bool(a | b);
            band = regulation::auditory_band(g, expressed_cells(w, slot, g.brain, c.tiles));
            auto nerve = watershed_forager(w, c, slot, band);
            weight_before = weight_after = nerve.contacts;
            if constexpr (compare || listen) {
                auto cold = nerve;
                cold.contacts = 0;
                cold_choice = capillary::reach(cold, light, mix(coin + 52673));
            }
            if constexpr (listen) {
                if (port == 2) {
                    auto reversed = nerve;
                    auto bare = nerve;
                    bare.contacts = 0;
                    other_light_move = capillary::reach(reversed, light ^ 1u, mix(coin + 52673));
                    other_bare = capillary::reach(bare, light ^ 1u, mix(coin + 52673));
                }
            }
            movement = capillary::reach(nerve, light, mix(coin + 52673));
            keep_foraging_spikes(w, c, slot, band, nerve);
            pump = port == 1 && movement && site.water >= threshold && behavior != 2 && c.weaving;
            if (pump) {
                a = 1;
                early = row |= 1;
                ++cost;
            }
        }
    }
    Word options = 0, configurations = 0, other_hand = 0, other_row = early, other_cast = 0, other_food = 0,
         other_cost = 0, other_prediction = capillary::absent;
    bool suitable = animal.body.pores && animal.body.limbs && valued && (!sorting || a || b);
    if (awake) {
        auto &m = w.capillaries[slot];
        original = m.tissue;
        Word gene = g.pigments[2], senses = capillary::channels(gene);
        capillary::Fit known{};
        if (contextual) {
            capillary::Body equipment{};
            equipment.dendrites = gene;
            equipment.motor = g.grip;
            old_history = w.material_history[slot];
            compartment = capillary::revise(m.tissue, old_history, equipment);
            known = compartment.fit;
        } else
            known = capillary::fit(m.tissue, gene, behavior != 4);
        Word best = 0, next_best = 0;
        capillary::Fit pooled{};
        if constexpr (compare)
            pooled = capillary::fit(m.tissue, gene, behavior != 4);
        for (Word grip = 0; grip <= 4; ++grip) {
            Word view = row;
            if (grip && (behavior == 2 || !c.weaving || !animal.body.limbs ||
                         !candidate_edge(view, site, animal, grip - 1, threshold, sorting)))
                continue;
            Count afferent = capillary::bit(view & senses);
            Word fresh = !((m.tissue.on | m.tissue.off) & afferent), tie = (mix(coin + grip * 2971) & 255) + 1;
            auto rank = [&](Word label, Count undecided, Word review, Count diagnostic) {
                Word score = Word(label == want && valued) * (3 + ((g.grip >> 11) & 15)) +
                             Word(label == capillary::absent) * ((g.grip >> 6) & 7) + fresh * (g.grip & 7) +
                             Word(bool(undecided & afferent)) * ((g.grip >> 20) & 7) +
                             Word(!grip) * ((g.grip >> 25) & 7);
                if (grip && review && (diagnostic & afferent))
                    score += (g.grip >> 8) & 31;
                return (score << 9) | tie;
            };
            Word label = capillary::predict(known.model, view & senses),
                 priority = rank(label, known.rivalry, compartment.review, compartment.diagnostic);
            if constexpr (compare) {
                options |= 1u << grip;
                configurations |= view << (6 * grip);
                Word prediction = capillary::predict(pooled.model, view & senses),
                     score = rank(prediction, pooled.rivalry, 0, 0);
                if (score > next_best) {
                    next_best = score;
                    other_hand = grip;
                    other_row = view;
                    other_prediction = prediction;
                }
            }
            if (priority > best) {
                best = priority;
                hand = grip;
                prediction = label;
            }
        }
        if constexpr (compare) {
            other_cast = suitable && (other_prediction == want ||
                                      (other_prediction == capillary::absent && ((coin >> 18) & 7) < (g.grip >> 29)));
            if constexpr (ports)
                if (port == 2)
                    other_cast = suitable && movement;
            other_cost = pump + Word(bool(other_hand)) + other_cast + Word(bool(animal.body.eyes));
            if (other_cast && Word(capillary::surge(other_row, stone)) == want)
                other_food = drift::low(
                    stock, drift::low(1u + __builtin_popcount(animal.body.limbs & 7), 65535u - energy + other_cost));
        }
        if (hand) {
            auto exchanged = exchange_capillary_piece(w, c, i, x, y, hand - 1, site, animal, sorting);
            edge0 = exchanged.edge0;
            edge1 = exchanged.edge1;
            cargo0 = exchanged.cargo0;
            cargo1 = exchanged.cargo1;
            donation = exchanged.donation;
            placed = !edge0;
            lifted = bool(edge0);
            ++cost;
            row = a | (b << 1) | (capillary_grain(site, threshold) << 2);
            learned_action = (capillary::predict(known.model, early & senses) != prediction);
        }
        cast = suitable && (prediction == want || (prediction == 2 && ((coin >> 18) & 7) < (g.grip >> 29)));
        if constexpr (ports)
            if (port == 2)
                cast = suitable && movement;
        cost += cast + Word(animal.body.eyes != 0);
        inferred = prediction != capillary::absent;
    }
    bool fired = capillary::surge(row, stone);
    Word live = n < 32 ? 65535u : ~0u;
    auto ballot = [&](Word b) {
        Word out;
        asm volatile("{ .reg .pred p; setp.ne.u32 p,%1,0; vote.sync.ballot.b32 %0,p,%2; }"
                     : "=r"(out)
                     : "r"(b), "r"(live));
        return out;
    };
    Word deposited = fired ? old_dye : paint, outgoing = fired ? paint : old_dye;
    Word wave = ballot(fired), lo = ballot(deposited & 1), hi = ballot(deposited & 2);
    if (deck.impulses) {
        Word changed = ballot(hand && capillary::surge(early, stone) != fired);
        if (!(i & 31))
            deck.impulses[i >> 5] = changed;
    }
    if (!(i & 31)) {
        next[i >> 5] = wave;
        pad[i >> 5] = lo;
        pad[words + (i >> 5)] = hi;
    }
    if (fired)
        w.braids.floats[i] = (w.braids.floats[i] & ~3u) | outgoing;
    if (awake) {
        animal.energy -= cost;
        animal.worked += cost;
        if (cast && Word(fired) == want) {
            bite =
                drift::low(stock, drift::low(1u + __builtin_popcount(animal.body.limbs & 7), 65535u - animal.energy));
            site.litter -= bite;
            animal.energy += bite;
            animal.meals += bite;
            animal.diet[2] += bite;
        }
        auto &m = w.capillaries[slot];
        if (animal.body.eyes) {
            if (c.plastic && behavior != 3) {
                Word band = motor_band(w, c, slot);
                auto soma = read_band(w, c.tiles, slot, band);
                auto cortex = regulation::source(g, band, soma);
                auto &h = w.braids.minds[slot];
                capillary::Body body{{h.genes.plastic >> 8, h.genes.arbor, cortex.program, 1, g.hearing.expression},
                                     cortex.cells,
                                     g.pigments[2],
                                     g.grip,
                                     1,
                                     Word(bool(animal.body.limbs)),
                                     Word(bool(animal.body.pores))};
                Word permission = 0;
                if (deck.law.mode & capillary::regional_synapses)
                    body.pressure_xor = capillary::pressure_regulator(g, expressed_cells(w, slot, g.brain, c.tiles));
                edits = capillary::material_receive(m.tissue, body, row, fired, spikes, 1, &permission);
                if (contextual) {
                    memory_gate = permission;
                    Word seen = row & capillary::channels(g.pigments[2]);
                    bool moved = hand && row != early;
                    tested = moved && bool(compartment.diagnostic & capillary::bit(seen));
                    written = tested && bool(permission & (fired ? 1 : 2));
                    capillary::remember(w.material_history[slot], seen, fired, moved, compartment.diagnostic,
                                        permission);
                }
                if (body.cells.spike != soma.spike) {
                    soma.spike = body.cells.spike;
                    assign_band(w, c.tiles, slot, band, soma);
                }
            }
            Word colours = old_dye | (paint << 2);
            capillary::retain(m, capillary::centroid(x, y), row & capillary::channels(g.pigments[2]), fired, hand,
                              colours, pair);
        }
        if constexpr (ports && compare) {
            if (port == 2) {
                cold_cast = suitable && cold_choice;
                Word alternative_heat = cost - cast + cold_cast;
                if (cold_cast && Word(fired) == want)
                    cold_food = drift::low(stock, drift::low(1u + __builtin_popcount(animal.body.limbs & 7),
                                                             65535u - energy + alternative_heat));
            }
        }
        if constexpr (ports) {
            if (port && c.plastic && behavior != 3 && !(deck.law.mode & capillary::fixed_appetite)) {
                auto nerve = watershed_forager(w, c, slot, band);
                // The pre/post trace records the action actually executed at the pore.
                food_edits = capillary::digest_signal(nerve, light, port == 1 ? pump : cast, bite != 0);
                weight_after = w.chemical_contacts[slot] = nerve.contacts;
                keep_foraging_spikes(w, c, slot, band, nerve);
            }
        }
        if constexpr (compare) {
            bool different = hand != other_hand || cast != other_cast;
            Word where = (i * 17183 + mix(c.seed + 23743)) & (n - 1);
            if (where < deck.context_sites && (written || different || bite != other_food)) {
                capillary::ContextNote note{original,
                                            old_history,
                                            w.material_history[slot],
                                            compartment.diagnostic,
                                            tick + 1,
                                            i,
                                            slot,
                                            animal.born,
                                            stone,
                                            g.pigments[2],
                                            g.grip,
                                            coin,
                                            options,
                                            configurations,
                                            want,
                                            Word(valued),
                                            Word(suitable),
                                            Word(bool(animal.body.eyes)),
                                            animal.body.limbs,
                                            stock,
                                            energy,
                                            hand,
                                            other_hand,
                                            cast,
                                            other_cast,
                                            row,
                                            other_row,
                                            bite,
                                            other_food,
                                            cost,
                                            other_cost,
                                            memory_gate,
                                            compartment.review,
                                            compartment.selected,
                                            written};
                note.port = port;
                note.choice = movement;
                note.pump = pump;
                if (written || different)
                    deck.context_notes[where] = note;
                if (bite != other_food)
                    deck.context_notes[deck.context_sites + where] = note;
            }
        }
        if (deck.samples) {
            Word rank = (i * 14821 + mix(c.seed ^ mix(index))) & (n - 1);
            if (rank < deck.samples) {
                deck.notes[rank] = {
                    tick + 1,
                    i,
                    slot,
                    animal.born,
                    stone,
                    early,
                    row & capillary::channels(g.pigments[2]),
                    row,
                    Word(fired),
                    hand,
                    prediction,
                    cast,
                    stock,
                    bite,
                    site.litter,
                    energy,
                    animal.energy,
                    cost,
                    cargo0,
                    cargo1,
                    edge0,
                    edge1,
                    site.water,
                    (w.braids.minds[slot].genes.plastic >> 8) ^
                        ((deck.law.mode & capillary::regional_synapses)
                             ? capillary::pressure_regulator(g, expressed_cells(w, slot, g.brain, c.tiles))
                             : 0u),
                    g.pigments[2],
                    g.grip,
                    original,
                    m.tissue,
                    old_dye,
                    paint,
                    outgoing,
                    deposited,
                    threshold,
                    want,
                    appetite,
                    donation};
                if constexpr (ports) {
                    auto &v = deck.notes[rank];
                    v.port = port;
                    v.light = light;
                    v.choice = movement;
                    v.pump = pump;
                    v.basal_row = basal_row;
                    v.contacts_before = weight_before;
                    v.contacts_after = weight_after;
                    v.food_edits = food_edits;
                    v.food_law = (w.braids.minds[slot].genes.plastic >> 8) & 15;
                    v.stock_threshold = 1 + ((g.stomach >> 12) & 15);
                    v.net_available = suitable;
                    v.mouthful = 1u + __builtin_popcount(animal.body.limbs & 7);
                    v.pump_available = site.water >= threshold && behavior != 2 && c.weaving;
                    if constexpr (compare) {
                        v.prior_choice = cold_choice;
                        v.shadow_cast = cold_cast;
                        v.shadow_food = cold_food;
                        v.appetite_audit = port != 0;
                    }
                }
                deck.notes[rank].geology = geology;
                deck.notes[rank].chemistry = reactive;
                if (contextual) {
                    auto &sample = deck.notes[rank];
                    sample.context_before = old_history;
                    sample.context_after = w.material_history[slot];
                    sample.alternatives = compartment.diagnostic;
                    sample.memory_gate = memory_gate;
                    sample.in_context = 1;
                    sample.review = compartment.review;
                    sample.chosen_context = compartment.selected;
                }
            }
        }
    }
    if constexpr (listen) {
        capillary::Reception r{};
        r.tick = tick + 1;
        r.destination = i;
        r.reader = slot;
        r.reader_born = animal.born;
        r.input = row;
        r.rock = stone;
        r.light = light;
        r.move = movement;
        r.other_move = other_light_move;
        r.bare = cold_choice;
        r.other_bare = other_bare;
        r.cast = cast;
        r.wants = want;
        r.available = animal.body.pores && animal.body.limbs && valued;
        r.sorting = sorting;
        r.stock = stock;
        r.mouthful = 1u + __builtin_popcount(animal.body.limbs & 7);
        r.energy = energy;
        r.heat = cost;
        r.food = bite;
        pass_chemical_pulse(deck.gallery, c.side, index, x, y, port, pump, basal_row, spring, tide != 0, fired, r);
    }
    if constexpr (record) {
        auto film = deck.film;
        Word px = (x - film.x) & (c.side - 1), py = (y - film.y) & (c.side - 1);
        if (px < film.side && py < film.side) {
            Word signals = early | (row << 6) | (stone << 12) | (hand << 17) | (Word(fired) << 20) |
                           (Word(spring || pump) << 21) | (Word(!tide) << 22) | (cast << 23) |
                           (Word(bool(energy)) << 24) | (learned_action << 25) | (Word(!want) << 26);
            Word possible = cast ? drift::low(stock, drift::low(1u + __builtin_popcount(animal.body.limbs & 7),
                                                                65535u - (energy - cost)))
                                 : 0;
            Word resources = stock | (bite << 16) | (possible << 20) | (old_dye << 24) | (outgoing << 26) |
                             (Word(placed != 0) << 28) | (Word(lifted != 0) << 29);
            if (reactive) {
                signals |= geology << 27;
                resources |= paint << 30;
            }
            film.marks[(Count(index % film.frames) * film.side + py) * film.side + px] = {slot, animal.born, signals,
                                                                                          resources};
        }
    }
    auto group = drift_warp(true, 0, false);
    auto &book = deck.ledger[(i >> 7) & 127];
    auto &heat = w.account[i >> 8].heat;
    drift_count(heat, cost, group);
#define AX_WAVE(field, val) drift_count(book.field, Word(val), group)
    AX_WAVE(steps, 1);
    AX_WAVE(springs, spring);
    AX_WAVE(carried, fired);
    AX_WAVE(attended, awake && animal.body.eyes);
    AX_WAVE(experiments, hand != 0);
    AX_WAVE(changed, early != row);
    AX_WAVE(placed, placed);
    AX_WAVE(lifted, lifted);
    AX_WAVE(nets, cast);
    AX_WAVE(caught, bite);
    AX_WAVE(correct, inferred && prediction == fired);
    AX_WAVE(forecast, inferred);
    AX_WAVE(edits, edits);
    AX_WAVE(spikes, spikes);
    AX_WAVE(contrasts, pair);
    AX_WAVE(heat, cost);
    AX_WAVE(return_nets, cast && !want);
    AX_WAVE(return_food, !want ? bite : 0);
    AX_WAVE(reused, donation != 0);
    AX_WAVE(model_actions, learned_action);
    AX_WAVE(model_food, learned_action * bite);
    if constexpr (ports) {
        Word without = (row & ~1u) | (basal_row & 1);
        AX_WAVE(writing, port == 1);
        AX_WAVE(reading, port == 2);
        AX_WAVE(pumped, pump);
        AX_WAVE(pump_effect, pump && capillary::surge(without, stone) != fired);
        AX_WAVE(chemical_nets, port == 2 && cast);
        AX_WAVE(writer_food, port == 1 ? bite : 0);
        AX_WAVE(reader_food, port == 2 ? bite : 0);
        AX_WAVE(appetitive_edits, food_edits);
    }
    if constexpr (ports && compare) {
        AX_WAVE(port_compared, port != 0);
        AX_WAVE(changed_preference, port && movement != cold_choice);
        AX_WAVE(changed_pumps,
                port == 1 && site.water >= threshold && behavior != 2 && c.weaving && movement != cold_choice);
        AX_WAVE(changed_nets, port == 2 && cast != cold_cast);
        AX_WAVE(port_extra_food, port == 2 && bite > cold_food ? bite - cold_food : 0);
        AX_WAVE(port_missed_food, port == 2 && cold_food > bite ? cold_food - bite : 0);
    }
    if (reactive) {
        bool inert = capillary::surge(row, geology);
        Word next_mineral = capillary::conditioned_mineral(geology, deposited, outgoing);
        bool effective_grip =
            hand && capillary::surge(early, stone) != fired && capillary::surge(early, geology) == inert;
        Word missed = cast && Word(inert) == want && Word(fired) != want
                          ? drift::low(stock, drift::low(1u + __builtin_popcount(animal.body.limbs & 7),
                                                         65535u - (energy - cost)))
                          : 0;
        AX_WAVE(mixed_material, stone != geology);
        AX_WAVE(pulse_added, fired && !inert);
        AX_WAVE(pulse_removed, !fired && inert);
        AX_WAVE(switched_sills, stone != next_mineral);
        AX_WAVE(one_shot, fired && stone != next_mineral && !capillary::surge(row, next_mineral));
        AX_WAVE(active_grips, effective_grip);
        AX_WAVE(enabled_bite, fired != inert ? bite : 0);
        AX_WAVE(foregone_bite, missed);
    }
    if (contextual) {
        AX_WAVE(ambiguity_seen, awake && bool(compartment.diagnostic));
        AX_WAVE(tests_due, awake && compartment.review);
        AX_WAVE(rechecked, tested);
        AX_WAVE(retained_tests, written);
        AX_WAVE(read_context, awake && compartment.selected);
    }
    if constexpr (compare) {
        AX_WAVE(comparisons, awake);
        AX_WAVE(different_hand, awake && hand != other_hand);
        AX_WAVE(different_net, awake && cast != other_cast);
        AX_WAVE(food_added, bite > other_food ? bite - other_food : 0u);
        AX_WAVE(food_forgone, other_food > bite ? other_food - bite : 0u);
        AX_WAVE(other_food, other_food);
        AX_WAVE(other_cost, other_cost);
    }
#undef AX_WAVE
}

KERNEL void flood_capillaries(World w, Configuration c, const Word *clock, Word offset, capillary::Device deck) {
    capillary_tick<false>(w, c, clock, offset, deck);
}
KERNEL void flood_capillary_film(World w, Configuration c, const Word *clock, Word offset, capillary::Device deck) {
    capillary_tick<true>(w, c, clock, offset, deck);
}
KERNEL void flood_capillary_context(World w, Configuration c, const Word *clock, Word offset, capillary::Device deck) {
    capillary_tick<false, true>(w, c, clock, offset, deck);
}
KERNEL void flood_capillary_context_film(World w, Configuration c, const Word *clock, Word offset,
                                         capillary::Device deck) {
    capillary_tick<true, true>(w, c, clock, offset, deck);
}
KERNEL void flood_capillary_context_audit(World w, Configuration c, const Word *clock, Word offset,
                                          capillary::Device deck) {
    capillary_tick<false, true, true>(w, c, clock, offset, deck);
}
KERNEL void flood_capillary_context_audit_film(World w, Configuration c, const Word *clock, Word offset,
                                               capillary::Device deck) {
    capillary_tick<true, true, true>(w, c, clock, offset, deck);
}
// Keep dormant habitats free of both port branches and their register allocation.
#define AX_FORAGING_KERNEL(name, film, adaptive, audit)                                                                \
    KERNEL void name(World w, Configuration c, const Word *clock, Word offset, capillary::Device d) {                  \
        capillary_tick<film, adaptive, audit, true>(w, c, clock, offset, d);                                           \
    }
AX_FORAGING_KERNEL(flood_capillaries_ports, false, false, false)
AX_FORAGING_KERNEL(flood_capillary_film_ports, true, false, false)
AX_FORAGING_KERNEL(flood_capillary_context_ports, false, true, false)
AX_FORAGING_KERNEL(flood_capillary_context_film_ports, true, true, false)
AX_FORAGING_KERNEL(flood_capillary_context_audit_ports, false, true, true)
AX_FORAGING_KERNEL(flood_capillary_context_audit_film_ports, true, true, true)
#undef AX_FORAGING_KERNEL
#define AX_CHEMICAL_WITNESS(name, film, adaptive, audit)                                                               \
    KERNEL void name(World w, Configuration c, const Word *clock, Word offset, capillary::Device d) {                  \
        capillary_tick<film, adaptive, audit, true, true>(w, c, clock, offset, d);                                     \
    }
AX_CHEMICAL_WITNESS(flood_capillaries_ports_wake, false, false, false)
AX_CHEMICAL_WITNESS(flood_capillary_film_ports_wake, true, false, false)
AX_CHEMICAL_WITNESS(flood_capillary_context_ports_wake, false, true, false)
AX_CHEMICAL_WITNESS(flood_capillary_context_film_ports_wake, true, true, false)
AX_CHEMICAL_WITNESS(flood_capillary_context_audit_ports_wake, false, true, true)
AX_CHEMICAL_WITNESS(flood_capillary_context_audit_film_ports_wake, true, true, true)
#undef AX_CHEMICAL_WITNESS
KERNEL void capillary_coatings(Word *physical, Word words, Word seed) {
    Word i = tid();
    if (i < words) {
        physical[2 * words + i] = mix(seed ^ mix(i + 14627));
        physical[3 * words + i] = mix(seed ^ mix(i + 83047));
    }
}
KERNEL void capillary_expand(Word *board, const Word *old, Word previous, Word side, Word phase) {
    Word i = tid(), n = side * side;
    if (i >= n)
        return;
    Word words = (n + 31) / 32, p = (previous * previous + 31) / 32, x = i & (side - 1), y = i / side;
    Word before = y * previous + x;
    for (Word j = 0; j < 3; ++j) {
        Word shift = j ? j + 1 : phase;
        Word offset = shift * words + (i >> 5);
        Word v = x < previous && y < previous ? (old[j * p + (before >> 5)] >> (before & 31)) & 1u
                                              : (board[offset] >> (i & 31)) & 1u;
        Word packed, mask = n < 32 ? 65535u : ~0u;
        asm volatile("{.reg .pred q; setp.ne.u32 q,%1,0; vote.sync.ballot.b32 %0,q,%2;}"
                     : "=r"(packed)
                     : "r"(v), "r"(mask));
        if (!(i & 31))
            board[offset] = packed;
    }
}
KERNEL void capillary_residents(World w, Configuration c, const Word *slots, Word n, capillary::Body *body,
                                capillary::Mind *prior, capillary::History *histories, unsigned char *appetites) {
    Word i = tid();
    if (i >= n)
        return;
    Word slot = slots[i];
    auto &g = w.dna[slot];
    auto &a = w.animals[slot];
    auto &h = w.braids.minds[slot];
    Word band = regulation::auditory_band(g, expressed_cells(w, slot, g.brain, c.tiles));
    auto cortex = regulation::source(g, band, read_band(w, c.tiles, slot, band));
    body[i] = {{h.genes.plastic >> 8, h.genes.arbor, cortex.program, 1, g.hearing.expression},
               cortex.cells,
               g.pigments[2],
               g.grip,
               Word(bool(a.body.eyes)),
               Word(bool(a.body.limbs)),
               Word(bool(a.body.pores))};
    body[i].palate = g.stomach;
    body[i].module_mask = capillary::pressure_regulator(g, expressed_cells(w, slot, g.brain, c.tiles));
    prior[i] = w.capillaries ? w.capillaries[slot] : capillary::Mind{};
    if (appetites)
        appetites[i] = w.chemical_contacts ? w.chemical_contacts[slot] : 0;
    if (histories)
        histories[i] = w.material_history ? w.material_history[slot] : capillary::History{};
}
KERNEL void capillary_quarry(World w, Configuration c, capillary::Settings weather, Word now, Word seed, Word depth,
                             Word scarce, Word n, capillary::Work *out) {
    Word j = tid();
    if (j >= n)
        return;
    Word key = mix(seed ^ mix(j * 3517 + 77893));
    auto fixture = capillary::channel(key, depth, scarce);
    Word ox = mix(key + 3727) & (c.side - 1), oy = mix(key + 1879) & (c.side - 1);
    fixture.bed = {};
    for (Word k = 0; k < 64; ++k) {
        Word x = (ox + (k & 7)) & (c.side - 1), y = (oy + (k >> 3)) & (c.side - 1);
        auto &s = w.land[y * c.side + x];
        Word patch = mix(c.seed ^ mix((x >> 2) + 11239u * (y >> 2)));
        Word tide = ((now >> weather.shift) + (patch >> 15)) & 31;
        Word t = 2 + ((patch >> 21) & 15) + 2 * (tide < 16 ? tide : 31 - tide), v = capillary_grain(s, t);
        for (Word h = 0; h < 4; ++h)
            if (v & (1u << h))
                capillary::surface(fixture.bed, h) |= capillary::bit(k);
    }
    out[j] = fixture;
}

KERNEL void capillary_donors(const capillary::Mark *marks, Word width, Word frames, Word begin, Word span,
                             Word duration, Word *count, capillary::Donor *origins) {
    Word i = tid(), area = width * width, j = i / area, k = i % area;
    if (j >= span - duration)
        return;
    Word epoch = begin + j;
    auto m = marks[Count(epoch % frames) * area + k];
    Word early = m.signals & 63, later = (m.signals >> 6) & 63, rock = (m.signals >> 12) & 31;
    if (early != later && capillary::surge(early, rock) != capillary::surge(later, rock)) {
        Word rank;
        asm volatile("atom.global.add.u32 %0,[%1],1;" : "=r"(rank) : "l"(count) : "memory");
        if (origins)
            origins[rank] = {epoch, k};
    }
}
KERNEL void capillary_wakes(const capillary::Mark *marks, Word width, Word frames, Word whole, Word duration,
                            const capillary::Donor *origins, Word n, capillary::Echo *out) {
    Word i = tid();
    if (i >= n)
        return;
    auto source = origins[i];
    Word area = width * width, x = source.position % width, y = source.position / width;
    auto donor = marks[Count(source.moment % frames) * area + source.position];
    capillary::Echo e{};
    e.influence[0] = 1;
    Word difference = 1;
    for (Word delay = 1; delay <= duration && difference; ++delay) {
        Word next = 0, epoch = source.moment + delay;
        for (Word dx = 0; dx <= delay; ++dx) {
            Word px = x + dx, py = y + delay - dx;
            if (!whole && (px >= width || py >= width))
                continue;
            Word at = (py % width) * width + (px % width);
            auto s = marks[Count(epoch % frames) * area + at];
            Word actual = (s.signals >> 6) & 63, alternate = actual;
            if (!(s.signals & (1u << 22))) {
                if (dx && !(s.signals & (1u << 21)))
                    alternate ^= (difference >> (dx - 1)) & 1;
                if (dx < delay)
                    alternate ^= ((difference >> dx) & 1) << 1;
            }
            Word real = (s.signals >> 20) & 1, variant = capillary::surge(alternate, (s.signals >> 12) & 31);
            if (real == variant)
                continue;
            next |= 1u << dx;
            e.enabled += real;
            e.suppressed += !real;
            bool other = s.slot != donor.slot || s.born != donor.born;
            if (!other)
                continue;
            Word eaten = (s.resources >> 16) & 15, offered = (s.resources >> 20) & 15;
            if (real != ((s.signals >> 26) & 1))
                e.other += eaten;
            else
                e.prevented += offered;
            if (eaten && delay >= e.horizon) {
                e.horizon = delay;
                e.site = at;
                e.slot = s.slot;
                e.born = s.born;
                e.when = epoch;
                e.net = eaten;
            }
        }
        e.influence[delay] = next;
        difference = next;
    }
    out[i] = e;
}

KERNEL void capillary_fragments(const capillary::Mark *film, Word width, Word frames, Word horizon,
                                const capillary::Donor *donors, Word n, capillary::Mark *out) {
    Word i = tid(), stride = (horizon + 1) * (horizon + 2) / 2, donor = i / stride;
    if (donor >= n)
        return;
    Word sample = i % stride, depth = 0;
    while (sample > depth)
        sample -= ++depth;
    auto root = donors[donor];
    Word px = (root.position % width + sample) % width, py = (root.position / width + depth - sample) % width;
    out[i] = film[(Count((root.moment + depth) % frames) * width + py) * width + px];
}

KERNEL void flood_catalytic_contacts(Word *receipts) {
    Word n = tid(), row = n & 63, carrier = (n >> 6) & 3, bound = (n >> 8) & 3, base = n >> 10;
    auto first = capillary::cycle(base, bound, carrier, row);
    auto second = capillary::cycle(base, first.coating, first.carrier, row);
    receipts[n] = first.before | (first.after << 5) | (first.flow << 10) | (second.flow << 11) | (first.carrier << 12) |
                  (first.coating << 14) | (second.carrier << 16) | (second.coating << 18);
}

KERNEL void catalytic_closures(capillary::Film film, Word begin, Word span, Word *count, capillary::Closing *events) {
    Word i = tid(), area = film.side * film.side, j = i / area, site = i % area;
    if (j + 1 >= span)
        return;
    Word epoch = begin + j;
    auto a = film.marks[Count(epoch % film.frames) * area + site],
         b = film.marks[Count((epoch + 1) % film.frames) * area + site];
    Word row = (a.signals >> 6) & 63, base = a.signals >> 27;
    if (!((a.signals >> 20) & 1) || ((b.signals >> 20) & 1) || row != ((b.signals >> 6) & 63) ||
        base != (b.signals >> 27))
        return;
    auto p = capillary::cycle(base, a.resources >> 30, (a.resources >> 24) & 3, row);
    if (!p.flow || p.before == p.after || p.carrier != ((b.resources >> 24) & 3) || p.coating != (b.resources >> 30) ||
        capillary::surge(row, p.after))
        return;
    Word rank;
    asm volatile("atom.global.add.u32 %0,[%1],1;" : "=r"(rank) : "l"(count) : "memory");
    if (events)
        events[rank] = {epoch, site, a, b};
}
