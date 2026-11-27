using Plan = std::array<Word, 4>;
struct Morph {
    Word count = 0, representative = ~0u, min_cells = ~0u, max_cells = 0, min_modules = ~0u, max_modules = 0;
    Word parents = 0, builders = 0, listeners = 0, expanded = 0, contracted = 0;
    Count intake[3]{};
};
struct OpeningMorphs {
    std::map<Plan, Word> counts;
    Word total = 0;
};
struct FrontierSurvey {
    static constexpr Word resolution = 32;
    Word origin, start, opening = 0;
    bool observed = false;

    void sample(const Habitat &h, bool final) {
        const Word side = h.header.configuration.side;
        if (side == origin && !observed)
            observed = true;
        if (side <= origin)
            return;
        const Word bins = std::min(resolution, side);
        std::array<Word, resolution * resolution> density{};
        std::set<Word> founders;
        Word established = 0, outside = 0, young = 0, furthest = 0, largest = 0, fibre_sites = 0;
        Count cells = 0;
        auto distance = [&](Word u) { return u < origin ? 0u : std::min(u - origin + 1, side - u); };
        for (Word pos = 0; pos < h.sites(); ++pos) {
            const Word x = pos % side, y = pos / side, slot = h.slots[pos];
            const bool annex = x >= origin || y >= origin;
            const auto &land = h.land[pos];
            fibre_sites += annex && (land.fibre_east || land.fibre_south);
            const auto &animal = h.bodies[slot];
            if (!animal.energy)
                continue;
            ++density[(y * bins / side) * bins + x * bins / side];
            if (!annex) {
                ++established;
                continue;
            }
            ++outside;
            young += animal.born > start;
            founders.insert(animal.founder);
            const Word expressed = 32 * h.conducting(slot);
            cells += expressed;
            largest = std::max(largest, expressed);
            furthest = std::max(furthest, distance(x) + distance(y));
        }
        if (!observed)
            opening = outside;
        std::cout << "frontier_census="
                  << (final      ? "end"
                      : observed ? "sample"
                                 : "begin")
                  << "  tick=" << h.header.tick << "  start=" << start << "  origin=" << origin << "  side=" << side
                  << "  new_sites=" << h.sites() - origin * origin << "  opening=" << opening
                  << "  core_residents=" << established << "  outside=" << outside << "  born_after=" << young
                  << "  founders=" << founders.size() << "  expressed_cells=" << cells << "  largest=" << largest
                  << "  furthest=" << furthest << "  fibre_sites=" << fibre_sites << '\n';
        observed = true;
        if (!final)
            return;
        for (Word y = 0; y < bins; ++y) {
            std::cout << "frontier_row=" << y << "  residents=";
            for (Word x = 0; x < bins; ++x)
                std::cout << (x ? "," : "") << density[y * bins + x];
            std::cout << '\n';
        }
    }
};
struct Lineages {
    std::map<Plan, Morph> morphs;
    Word plant_genes = 0, root_policies = 0, marked[2]{};
    Count reserve[2]{};
    Word families = 0, forms = 0, tapes = 0, wirings = 0, body_clades = 0, brain_clades = 0, stable_bodies = 0,
         stable_brains = 0, specialists = 0, modules = 0, diet[3]{}, builders = 0, parents = 0, depth = 0, best = ~0u;
};
Lineages descendants(const Habitat &h) {
    std::set<Count> flora;
    std::set<Word> physiology;
    std::set<Word> founders;
    std::set<Plan> wires;
    std::set<std::array<Word, 9>> programs;
    std::map<Count, Word> body, brain;
    Lineages l;
    for (Word i = 0; i < h.sites(); ++i) {
        auto &plant = h.land[i];
        flora.insert((Count(plant.root) << 8) | plant.spore);
        physiology.insert(plant.root & 0xffffffu);
        auto &a = h.bodies[i];
        if (!a.energy)
            continue;
        auto &g = h.genomes[i].brain;
        if (a.cohort) {
            ++l.marked[a.cohort - 1];
            l.reserve[a.cohort - 1] += a.energy;
        }
        l.builders += a.laid != 0;
        l.parents += a.offspring != 0;
        if (a.meals) {
            Word best = Word(std::max_element(a.diet, a.diet + 3) - a.diet);
            l.diet[best] += Count(a.diet[best]) * 5 >= Count(a.meals) * 4;
        }
        founders.insert(a.founder);

        std::array<Word, 9> tape{};
        tape[8] = 1 + (g.expression & 7);
        for (Word j = 0; j < tape[8]; ++j)
            tape[j] = g.op[j] & 0x1fffffu;
        Word selected[4]{}, distinct = 0, used = 0;
        auto &organisation = h.genomes[i];
        Word tiles = h.conducting(i);
        for (Word t = 0; t < tiles; ++t) {
            Word k = ((organisation.allocation >> (2 * (t & 15))) & 3) % organisation.module_count;
            if (used & (1u << k))
                continue;
            used |= 1u << k;
            auto mod = organisation.module[k];
            auto gate = [](NeuralModule a) { return a.gate & (1u << 21) ? a.gate & 0x3fffffu : 0u; };
            bool duplicate = false;
            for (Word a = 0; a < distinct; ++a) {
                auto prev = organisation.module[selected[a]];
                duplicate |= mod.routing == prev.routing && gate(mod) == gate(prev);
            }
            if (!duplicate)
                selected[distinct++] = k;
        }
        auto &type = l.morphs[{a.body.shape, a.body.limbs, a.body.pores, a.body.eyes}];
        ++type.count;
        Word cells = 32 * tiles;
        type.min_cells = std::min(type.min_cells, cells);
        type.max_cells = std::max(type.max_cells, cells);
        type.min_modules = std::min(type.min_modules, distinct);
        type.max_modules = std::max(type.max_modules, distinct);
        if (h.developing) {
            const auto &stage = h.cortices[i];
            type.expanded += stage.active > stage.natal;
            type.contracted += stage.active < stage.natal;
        }
        type.parents += a.offspring != 0;
        type.builders += a.laid != 0;
        type.listeners += a.witnesses != 0;
        for (Word j = 0; j < 3; ++j)
            type.intake[j] += a.diet[j];
        if (type.representative == ~0u || a.born < h.bodies[type.representative].born)
            type.representative = i;
        l.modules += distinct;
        l.specialists += distinct > 1;
        programs.insert(tape);
        wires.insert({g.edges[0], g.edges[1], g.receptors, g.motors});
        if (a.body_origin >> 32)
            ++body[a.body_origin];
        if (a.brain_origin >> 32)
            ++brain[a.brain_origin];
        l.depth = std::max(l.depth, a.generation);
        if (l.best == ~0u || a.energy > h.bodies[l.best].energy)
            l.best = i;
    }
    auto persistent = [&](const auto &clades) {
        Word n = 0;
        for (auto [id, count] : clades)
            n += count >= 2 && h.header.tick - Word(id >> 32) >= h.header.configuration.season;
        return n;
    };
    l.plant_genes = Word(flora.size());
    l.root_policies = Word(physiology.size());
    l.families = Word(founders.size());
    l.forms = Word(l.morphs.size());
    l.tapes = Word(programs.size());
    l.wirings = Word(wires.size());
    l.body_clades = Word(body.size());
    l.brain_clades = Word(brain.size());
    l.stable_bodies = persistent(body);
    l.stable_brains = persistent(brain);
    return l;
}
void photograph(const Habitat &h, Basin &basin, const Lineages &stats, const OpeningMorphs &opening) {
    auto *j = journal::current();
    if (!j)
        return;
    auto c = h.header.configuration;
    Word side = c.side, dim = std::min(32u, side), stride = side / dim;
    const char *digit = "0123456789abcdef";
    for (Word y = 0; y < dim; ++y) {
        std::string row, material;
        for (Word x = 0; x < dim; ++x) {
            Count water = 0, fruit = 0, walls = 0, heads = 0, fibre = 0;
            for (Word dy = 0; dy < stride; ++dy)
                for (Word dx = 0; dx < stride; ++dx) {
                    Word pos = x * stride + dx + (y * stride + dy) * side;
                    auto p = h.land[pos];
                    water += p.water;
                    fruit += p.fruit + p.leaf / 4;
                    walls += p.east + p.south;
                    fibre += p.fibre_east + p.fibre_south;
                    heads += h.bodies[h.slots[pos]].energy != 0;
                }
            Count area = stride * stride;
            row += digit[std::min(Count(15), water / (5 * area))];
            row += digit[std::min(Count(15), fruit / (4 * area))];
            row += digit[std::min(Count(15), (walls + area - 1) / area)];
            row += digit[(15 * heads) / area];
            material += digit[std::min(Count(15), (fibre + area - 1) / area)];
        }
        j->line("watershed_row=" + std::to_string(y) + "  channels=" + row + "  fibre=" + material);
    }
    if (h.investigatory) {
        // Centre a full-resolution window on the oldest mark-reading resident.
        Word focus = 0, oldest = 0;
        for (Word pos = 0; pos < h.sites(); ++pos) {
            Word id = h.slots[pos], age = h.header.tick - h.bodies[id].born;
            if (h.bodies[id].energy && h.enquiries[id].last_seen && age > oldest) {
                oldest = age;
                focus = pos;
            }
        }
        Word width = std::min(10u, side);
        j->line("worksite_window=" + std::to_string(focus) + "  side=" + std::to_string(side) +
                "  width=" + std::to_string(width));
        for (Word y = 0; y < width; ++y) {
            std::string sites;
            for (Word x = 0; x < width; ++x) {
                Word pos = ((focus % side + x - width / 2) & (side - 1)) +
                           side * ((focus / side + y - width / 2) & (side - 1));
                Word mark = h.patina[pos].events & 255;
                sites += digit[mark >> 4];
                sites += digit[mark & 15];
                sites += digit[(h.mineral_bed[pos].faces ? 1 : 0) | (h.bodies[h.slots[pos]].energy ? 2 : 0)];
            }
            j->line("worksite_row=" + std::to_string(y) + "  cells=" + sites);
        }
    }
    if (h.saline) {
        Word focus = 0, oldest = 0;
        for (Word pos = 0; pos < h.sites(); ++pos) {
            Word id = h.slots[pos];
            if (h.bodies[id].energy && h.header.tick - h.bodies[id].born >= oldest) {
                focus = pos;
                oldest = h.header.tick - h.bodies[id].born;
            }
        }
        Word id = h.slots[focus];
        auto &a = h.saline_animals[id];
        std::ostringstream scope;
        scope << "brine_specimen=" << id << "  living=" << bool(h.bodies[id].energy) << "  age=" << oldest
              << "  site=" << focus << "  side=" << side << "  active=" << a.genes.active << "  low=" << a.memory.low
              << "  high=" << a.memory.high << "  weights=" << a.genes.readout << "  firing=";
        for (Word j = 0; j < 16; ++j)
            scope << (j ? "," : "") << a.genes.receptive[j];
        j->line(scope.str());
        for (Word y = 0; y < 8; ++y) {
            std::string sites;
            for (Word x = 0; x < 8; ++x) {
                Word pos = ((focus % side + x - 4) & (side - 1)) + side * ((focus / side + y - 4) & (side - 1));
                Word solution = brine::phase(h.salts[pos], h.land[pos].water);
                sites += digit[solution >> 4];
                sites += digit[solution & 15];
                sites += digit[Word(h.mineral_bed[pos].faces != 0) + 2 * Word(h.bodies[h.slots[pos]].energy != 0)];
            }
            j->line("brine_row=" + std::to_string(y) + "  states=" + sites);
        }
    }
    std::vector<Plan> rank;
    Word living = 0;
    for (auto &[plan, type] : stats.morphs) {
        rank.push_back(plan);
        living += type.count;
    }
    auto order = [&](const Plan &a, const Plan &b) {
        auto n = stats.morphs.at(a).count, m = stats.morphs.at(b).count;
        return n == m ? a < b : n > m;
    };
    std::partial_sort(rank.begin(), rank.begin() + std::min<size_t>(8, rank.size()), rank.end(), order);
    j->metrics["ranked_residents"] = living;
    j->metrics["ranked_morphs"] = rank.size();
    j->metrics["opening_residents"] = opening.total;
    j->metrics["census_tick"] = h.header.tick;
    for (size_t k = 0; k < std::min<size_t>(8, rank.size()); ++k) {
        const auto &plan = rank[k];
        const auto &m = stats.morphs.at(plan);
        Word id = m.representative;
        const auto &a = h.bodies[id];
        journal::Specimen p;
        p.role = "morphospecies";
        std::ostringstream name;
        name << "Morph " << std::hex << std::setfill('0') << std::setw(16)
             << storage::digest(storage::hash_start, plan.data(), sizeof(plan));
        p.path = name.str();
        auto prior = opening.counts.find(plan);
        p.measures = {{"rank", k + 1},
                      {"members", m.count},
                      {"opening_members", prior == opening.counts.end() ? 0 : prior->second},
                      {"body_plan", plan[0]},
                      {"legs", plan[1]},
                      {"pores", plan[2]},
                      {"eyes", plan[3]},
                      {"min_neurons", m.min_cells},
                      {"max_neurons", m.max_cells},
                      {"min_modules", m.min_modules},
                      {"max_modules", m.max_modules},
                      {"leaf", m.intake[0]},
                      {"fruit", m.intake[1]},
                      {"litter", m.intake[2]},
                      {"builders", m.builders},
                      {"parents", m.parents},
                      {"listeners", m.listeners},
                      {"slot", id},
                      {"age", h.header.tick - a.born},
                      {"generation", a.generation},
                      {"energy", a.energy},
                      {"heading", a.heading},
                      {"offspring", a.offspring},
                      {"motions", a.motions},
                      {"calls_heard", a.witnesses},
                      {"body", h.genomes[id].brain.body}};
        if (h.developing) {
            auto growth = h.cortices[id];
            auto &q = p.measures;
            q["natal_cells"] = 32 * growth.natal;
            q["peak_cells"] = 32 * growth.peak;
            q["larger_than_birth"] = m.expanded;
            q["smaller_than_birth"] = m.contracted;
            q["least_cells"] = 32 * growth.least;
            q["active_cells"] = 32 * h.conducting(id);
            q["muted_cells"] = 32 * growth.active - q["active_cells"];
            q["capacity_cells"] = 32 * c.tiles;
        }
        if (h.articulated) {
            const auto &worker = h.workers[id];
            auto &q = p.measures;
            q["hand_faces"] = worker.hand.faces;
            q["hand_material"] = worker.hand.substance;
            q["hand_charge"] = worker.hand.charge;
            q["work_sightings"] = worker.sightings;
            q["hand_edits"] = worker.edits;
            q["manual_ports"] = worker.genes.receptors >> 24;
        }
        if (h.foresight) {
            const auto &memory = h.forecasts[id];
            p.measures["seen_face_pairs"] = std::popcount(memory.appetite >> 16);
            p.measures["predicted_contacts"] = std::popcount(memory.contacts);
            p.measures["observed_motors"] = std::popcount(memory.muscles[0]) + std::popcount(memory.muscles[1]);
            p.measures["spatial_replay_bytes"] = sizeof(memory);
        }
        if (h.investigatory) {
            auto &inq = h.enquiries[id];
            p.measures["explored_substrates"] = std::popcount(inq.sampled & 65535u);
            if (h.saline) {
                auto &a = h.saline_animals[id];
                p.measures["predictive_cells"] = std::popcount(a.genes.active);
                p.measures["modified_predictive_cells"] = std::popcount(a.memory.low | a.memory.high);
            }
            p.measures["empty_palm_trials"] = std::popcount((inq.sampled >> 16) & 255u);
            p.measures["loaded_palm_trials"] = std::popcount(inq.sampled >> 24);
        }
        if (h.calling) {
            auto &c = h.callers[id];
            p.measures["siphon_contacts"] = c.occasions;
            p.measures["siphon_heard"] = c.listened;
            p.measures["siphon_food"] = c.harvest;
            auto wiring = siphon::settle(c);
            p.measures["acoustic_address_bits"] = std::popcount(c.address) + std::popcount(c.surface);
            p.measures["acoustic_tendon_bits"] = std::popcount(c.tendon);
            p.measures["acoustic_inquiry"] = wiring.ask;
            p.measures["acoustic_reply"] = wiring.speak;
        }
        if(h.braiding){
            if (h.drift_water.law.ports & drift::lateral) {
                const auto &hand = h.riggers[id];
                const auto &g = h.genomes[id];
                Word ports = h.drift_water.law.ports, width = h.conducting(id);
                Word band = regulation::innervated_band(g, width), cohort = braid::drift_tag(hand.lived);
                auto &q = p.measures;
                q["inlet_contacts"] = hand.volley >> 16;
                q["inlet_wiring"] = drift::wiring(hand.genes, ports);
                q["inlet_reads"] = drift::reads(ports, cohort);
                q["inlet_distal"] = bool(drift::branch(ports, cohort) & drift::lateral);
                q["inlet_motor_band"] = band;
                q["inlet_vocal_band"] = ports & drift::auditory ? regulation::auditory_band(g, width) : band;
            }
            if(h.loom.version>=5){
                auto eye=braid::eye_wiring(h.loom.mode,h.riggers[id].genes,h.bodies[id].heading);
                p.measures["retinal_positions"]=eye.places;p.measures["retinal_bands"]=eye.hues;
            }
            if(h.recording_floats){
                p.measures["visible_event_ports"]=std::popcount(h.float_events[id].known);
                for(auto &voice:h.narrators)if(voice.slot==id && voice.born==h.bodies[id].born){
                        Word forms = 0;
                        for (auto &form : voice.mind.forms)
                            forms += form.examples != 0 && !braid::sequence_form(form);
                        p.measures["question_forms"] = forms;
                        break;
                }
            }
            if(h.loom.version>=3)p.measures["pressure_pairs"]=std::popcount(h.sluice_contacts[id].visited);
            const auto &m=h.riggers[id];p.measures["float_word_cells"]=std::popcount(Word((m.lived>>4)&255));
            p.measures["float_motor_cells"]=std::popcount(m.lived&15u);p.measures["float_capacity"]=braid::capacity(m.genes);
            p.senses.emplace_back("Moving float pigments");p.circuits.emplace_back("Convergent motion and auditory assemblies");
        }
        if(h.rattling){
            auto &m=h.listeners[id];p.measures["rattle_contacts"]=rattle::contacts(m);
            p.measures["rattle_timbre"]=(m.genes.drum>>16)&3;p.measures["rattle_latency"]=rattle::latency(m.genes);
            p.senses.emplace_back("Local vibrations and delayed taste");p.circuits.emplace_back("Private resonator contacts");
        }
        if(h.reeding){
            const auto &m=h.cutters[id];p.measures["flex_contacts"]=runnel::population(m.cells.touched);p.measures["witnessed_folds"]=m.practice>>16;
            p.measures["private_folds"]=m.practice&65535;p.measures["peer_contacts"]=runnel::population(m.cells.witnessed);
            p.senses.emplace_back("Visible deflection of a folded strand");p.circuits.emplace_back("Coincident groove, hand and deflection spikes");
        }
        if(h.practising){
            auto &m=h.pupils[id];p.measures["learned_contexts"]=practice::size(m);p.measures["context_limit"]=practice::capacity(m.genes,h.learning.arena);
            p.measures["personal_observations"]=m.samples;p.measures["attention_span"]=practice::span(m.genes);
            p.circuits.emplace_back("Competing material hypotheses and sustained motor engagement");
        }
        if(h.linking){
            auto &m=h.makers[id];Word banks=0;for(Word j=0;j<4;++j)banks+=!!linkage::key_at(m,j);
            p.measures["material_contexts"]=banks;p.measures["context_capacity"]=linkage::expressed(m,h.millwork.arena);
            p.measures["mechanism_contacts"]=m.contacts;p.measures["motor_horizon"]=linkage::horizon(m.genes);
            p.circuits.emplace_back("Material, coupling and assembly contacts");
            if(m.genes.arbor&(1u<<11))p.senses.emplace_back("Visible coupling effects of another body");
        }
        if(h.appraising){
            auto &m=h.economies[id];
            p.measures["learned_yields"]=std::popcount(m.experience&15);
            p.measures["learned_costs"]=std::popcount((m.experience>>4)&15);
            p.measures["surface_transitions"]=std::popcount(m.changes.low|m.changes.high);
            p.measures["cost_memory_bytes"]=sizeof(m);p.measures["pending_route_ticks"]=effort::remaining(m);
            p.circuits.emplace_back("Prospective effort and yield contacts");
        }
        if(h.ranging) {
            const auto &m=h.wanderers[id];Word active=0,flavors=0;
            for(Word j=0;j<m.used;++j){auto e=h.bearings[Count(j)*h.sites()+id];if(hollow::strength(e)){++active;flavors|=1u<<hollow::identity(e);}}
            p.circuits.emplace_back("Plastic spatial event cells");
            if(m.genes.compass&(1u<<3))p.senses.emplace_back("Bodily turn impulses");
            if(m.genes.compass&(1u<<8))p.senses.emplace_back("Local surface recognition");
            p.measures["place_cells"]=active;p.measures["place_capacity"]=hollow::capacity(m,h.recess.slots);p.measures["place_events"]=flavors;
        }
        if (h.pottery) {
            p.measures["clay_assemblies"] = clay::count(h.potters[id]);
            p.measures["clay_capacity"] = clay::capacity(h.potters[id]);
            p.measures["clay_grooves"] = clay::size(h.cargo[id]);
            p.measures["clay_colors"] = h.potters[id].colors;
            p.measures["clay_active"] = clay::active(h.potters[id]);
            if (h.workshop.use & press::installed) {
                p.measures["tool_contacts"] = h.potters[id].charge >> 16;
                p.measures["bath_contacts"] = h.potters[id].banks >> 16;
            }
        }
        basin.retinal_portrait(p, h, id);
        basin.pod_portrait(p, id, a.cohort);
        j->specimens.emplace_back(std::move(p));
    }
    if (stats.best == ~0u)
        return;
    Word slot = stats.best;
    auto &a = h.bodies[slot];
    auto &g = h.genomes[slot];
    auto pos = Word(std::find(h.slots.begin(), h.slots.end(), slot) - h.slots.begin());
    journal::Specimen p;
    p.path = "Floodplain resident " + std::to_string(slot);
    p.role = "floodplain";
    p.senses = {"Local material and water", "Energy and carried sediment"};
    if (a.body.eyes)
        p.senses.emplace_back("Projecting photoreceptors");
    if ((c.wire & 3) && a.witnesses)
        p.senses.emplace_back("Acoustic reception");
    p.circuits = {"Somatic spiking circuit", "Heritable recurrent afferents", "Gated synapses"};
    if (a.witnesses)
        p.circuits.emplace_back("Cross-situational contacts");
    if (g.module_count > 1)
        p.circuits.emplace_back("Duplicated neural populations");
    p.measures = {{"body", g.brain.body},
                  {"body_plan", a.body.shape},
                  {"legs", a.body.limbs},
                  {"pores", a.body.pores},
                  {"eyes", a.body.eyes},
                  {"neurons", 32 * h.conducting(slot)},
                  {"modules", std::popcount(a.body.shape)},
                  {"neural_modules", g.module_count},
                  {"tape", 1 + (g.brain.expression & 7)},
                  {"generation", a.generation},
                  {"founder", a.founder},
                  {"energy", a.energy},
                  {"digested", a.meals},
                  {"offspring", a.offspring},
                  {"walls_laid", a.laid},
                  {"position", pos}};
    basin.pod_portrait(p, slot, a.cohort);
    j->specimens.emplace_back(std::move(p));
}
void describe(const Habitat &h, Basin &basin, bool portrait, OpeningMorphs &opening, bool initial = false) {
    auto l = descendants(h);
    std::cout << "flood_census=" << h.header.tick << "  families=" << l.families << "  distinct_bodies=" << l.forms
              << "  expressed_tapes=" << l.tapes << "  wirings=" << l.wirings
              << "  derived_body_clades=" << l.body_clades << "  derived_brain_clades=" << l.brain_clades
              << "  persistent_bodies=" << l.stable_bodies << "  persistent_brains=" << l.stable_brains
              << "  leaf_specialists=" << l.diet[0] << "  fruit_specialists=" << l.diet[1]
              << "  litter_specialists=" << l.diet[2] << "  living_builders=" << l.builders
              << "  parents=" << l.parents << "  producer_genotypes=" << l.plant_genes
              << "  producer_tables=" << l.root_policies << "  cohort_a=" << l.marked[0]
              << "  cohort_b=" << l.marked[1] << "  reserve_a=" << l.reserve[0] << "  reserve_b=" << l.reserve[1]
              << "  distinct_module_sum=" << l.modules << "  multi_module_residents=" << l.specialists
              << "  deepest=" << l.depth << '\n';
    if (initial)
        for (auto &[plan, type] : l.morphs) {
            opening.counts[plan] = type.count;
            opening.total += type.count;
        }
    if (portrait)
        photograph(h, basin, l, opening);
}
storage::AgentImage extract_individual(const Habitat &h, Word requested, bool observer) {
    auto pop = descendants(h);
    Word slot = pop.best;
    if (observer && slot != ~0u)
        for (Word k = 0; k < h.sites(); ++k)
            if (h.bodies[k].energy && h.bodies[k].witnesses > h.bodies[slot].witnesses)
                slot = k;
    if (requested != ~0u) {
        if (requested >= h.sites() || !h.bodies[requested].energy)
            throw std::invalid_argument("the requested resident slot is vacant");
        slot = requested;
    }
    if (slot == ~0u)
        throw std::invalid_argument("an empty floodplain has no resident to export");
    const auto &g = h.genomes[slot];
    const auto &life = h.bodies[slot];
    const auto &ear = h.ears[slot];
    auto c = h.header.configuration;
    storage::AgentImage resident;
    auto &core = resident.brain;
    core.genes = g.brain;
    core.tiles = c.tiles;
    core.bits = 1;
    core.outcome_lag = 1;
    core.seed = c.seed;
    core.ticks = h.header.tick - life.born;
    for (Word i = 0; i < c.tiles; ++i)
        core.tissue[i] = h.nerves[Count(slot) * c.tiles + i];
    auto &m = resident.referent;
    resident.grounded = true;
    m.genes = g.hearing;
    if(h.linking){resident.invents=true;resident.mechanism.mind=h.makers[slot];resident.mechanism.arena=h.millwork.arena;
        for(Word j=0;j<h.millwork.arena;++j)resident.mechanism.tissue[j]=h.mechanisms[Count(j)*h.sites()+slot];}
    if(h.braiding){
        if(!h.cap_memories.empty()){
            resident.hydraulic=true;resident.channels.mind=h.cap_memories[slot];
            for(auto &p:h.audible_residents)if(p.slot==slot&&p.born==life.born){resident.voices_signs=resident.compound_signals=resident.material_signals=true;resident.signal_voice=p.speech;break;}
            if(h.cap_law.mode&capillary::material_hands)resident.compound_signals=resident.material_signals=true;
            for(auto &p:h.compound_residents)if(p.slot==slot&&p.born==life.born){resident.compound_signals=resident.material_signals=true;resident.compound_contacts=p.signs;break;}
            if(!h.appetites.empty()){resident.material_signals=true;resident.appetitive=capillary::expand_signs(h.appetites[slot],g.pigments[2]);}
            if(!h.cap_histories.empty()){resident.channels.history=h.cap_histories[slot];resident.channels.adaptive=(h.cap_law.mode&capillary::regional_synapses)?3:(h.cap_law.mode&capillary::contextual)?1:2;}
            for(auto &person:h.cap_speakers)if(person.slot==slot && person.born==life.born){resident.channels.speech=person.speech;break;}
        }
        if (!h.trails.empty()) {
            resident.accounts.trail=h.trails[slot];resident.describes=true;
            if(h.steady_eyes())resident.accounts.tag=h.loom.mode&braid::individual_eyes?utterance::fixed_personal:utterance::fixed_shared;
        }
        for (const auto &p:h.stories) if(p.slot==slot && p.born==life.born) {
            resident.accounts.memory=p.memory;resident.describes=true;break;
        }
        if(h.loom.version>=3){resident.registers_gates=true;resident.sluice.gates=h.sluice_contacts[slot];}
        resident.binds_floats=true;resident.braided.mind=h.riggers[slot];
        if(h.recording_floats){
            auto &m=resident.braid_report.memory;
            for(auto &p:h.narrators)if(p.slot==slot&&p.born==life.born){m=p.mind;resident.narrates_floats=true;break;}
            const auto &event=h.float_events[slot];
            if(event.stamp && (!resident.narrates_floats || event.stamp!=m.event.stamp || event.images!=m.event.images || event.known!=m.event.known)){
                braid::experience(m,event);resident.narrates_floats=true;
            }
        }
        resident.braid_report.memory.auditory_route =
            bool(h.drift_water.law.ports & drift::auditory);
        if (resident.braid_report.memory.auditory_route)
            resident.narrates_floats = true;
        for(const auto &words:h.flute_words)if(words.slot==slot && words.born==life.born)resident.braided.words=words.lexicon;
    }
    if(h.rattling){resident.hears_banks=true;resident.rattle_organ.mind=rattle::rebase(h.listeners[slot],h.header.tick,0);}
    if(h.reeding){resident.folds_reeds=true;resident.reed.mind=h.cutters[slot];}
    if(h.practising){resident.practises=true;resident.training={};resident.training.mind=h.pupils[slot];resident.training.mind.focus=0;
        resident.training.arena=h.learning.arena;for(Word j=0;j<h.learning.arena;++j)resident.training.bank[j]=h.memories[Count(j)*h.sites()+slot];}
    if(h.appraising){resident.values_effort=true;resident.prospects.mind=h.economies[slot];}
    if(h.ranging) {
        for (const auto &q:h.place_words) if(q.slot==slot && q.born==h.bodies[slot].born) {
            resident.path_speech.words=q.words;resident.referent.genes=q.genes;break;
        }
        resident.ranges=true; resident.path_memory.mind=h.wanderers[slot];resident.path_memory.slot_limit=h.recess.slots;
        for(Word j=0;j<h.recess.slots;++j) resident.path_memory.marks[j]=h.bearings[Count(j)*h.sites()+slot];
    }
    if (h.pottery) {
        resident.potter = true;
        resident.ceramic.mind = h.potters[slot];
        resident.ceramic.palm = h.cargo[slot];
    }
    if (h.calling) {
        resident.calling = true;
        resident.calls.animal = h.callers[slot];
    }
    if (h.saline) {
        resident.saline = true;
        resident.brine_mind.cortex = h.saline_animals[slot];
    }
    if (h.investigatory) {
        resident.investigates = true;
        resident.inquiry = h.enquiries[slot];
    }
    if (h.foresight) {
        resident.projects = true;
        resident.forecast = h.forecasts[slot];
    }
    if (h.articulated) {
        resident.handling = true;
        resident.worker = h.workers[slot];
    }
    if (h.developing) {
        resident.grows_cells = true;
        resident.cortex = h.cortices[slot];
    }
    if (h.enclosed) {
        resident.delayed_pods = true;
        resident.pod_organ = h.pod_animals[slot];
    }
    resident.riverine = true;
    resident.resident.germ = g;
    resident.resident.tick = h.header.tick;
    resident.resident.animal = life;
    resident.resident.ear = ear;
    resident.resident.slot = slot;
    const char *voices[]{"ka", "ri", "po", "se", "vu", "lo", "mi", "za"};
    for (Word j = 0; j < call_types; ++j) {
        auto &w = m.word[++m.count];
        w.wave = acoustic::pulse(voices[j]);
        w.exposures = ear.visits[j];
        Word connected = grounding::release({ear.low[j], ear.high[j]}, g.hearing.high & 15);
        w.viable = ear.visits[j] ? std::popcount(connected) : 0;
        if (!w.viable)
            continue;
        for (Word light = 0; light < 256; ++light) {
            Word feature = pigments(g, light), v = 1u << (light & 31);
            if (feature & connected)
                w.possible[light / 32] |= v;
            if (!(connected & ~feature))
                w.certain[light / 32] |= v;
        }
    }
    Word pos = Word(std::find(h.slots.begin(), h.slots.end(), slot) - h.slots.begin());
    grounding::View view;
    auto expose = [&](Word place, Word retinal) {
        grounding::illuminate(view.after, retinal, optical(h.land[place], h.bodies[h.slots[place]].energy));
    };
    expose(pos, 27);
    const int displacement[]{-8, 1, 8, -1};
    auto dir = life.heading;
    Word reach = segment(life.body.eyes, dir), fore = pos;
    for (Word j = 0; j < reach; ++j)
        fore = neighbor(fore, dir, c.side);
    if (reach)
        expose(fore, Word(27 + int(reach) * displacement[dir]));
    if (life.body.eyes && (g.growth & (1u << 8))) {
        Word left = (dir + 3) & 3;
        expose(neighbor(pos, left, c.side), Word(27 + displacement[left]));
    }
    m.sights = 0;
    for (const auto &spoken : h.spoken)
        if (spoken.slot == slot && spoken.born == life.born) {
            m = spoken.vocabulary;
            resident.field_speech = spoken.questions;
            break;
        }
    for (const auto &p : h.captions)
        if (p.slot == slot && p.born == life.born) {
            resident.native_words = p.words;
            m.genes = p.genes;
            break;
        }
    m.here = grounding::see(view, m.genes);
    ++m.sights;
    m.extent = sizeof(m);
    grounding::validate(m);
    return resident;
}
void export_individual(const Habitat &h, const std::string &path, Word requested, bool observer) {
    auto resident = extract_individual(h, requested, observer);
    Word slot = resident.resident.slot;
    const auto &life = h.bodies[slot];
    const auto &g = h.genomes[slot];
    storage::write_checked(path, resident);
    std::cout << "field_extraction=" << slot << "  exposures=" << life.witnesses << "  born=" << life.born
              << "  generation=" << life.generation << "  active_cells=" << 32 * h.conducting(slot)
              << "  modules=" << g.module_count << '\n';
}
