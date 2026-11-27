Word current_wave() const {
    Word mask = (1u << capillary_field.law.shift) - 1;
    return Word(((Count(tick) + mask) >> capillary_field.law.shift) & 1);
}
void move_capillaries(Habitat &h, bool upload) {
    if (!cap_neurons.ptr)
        return;
    transfer_hands(h,upload);
    Word words = (n + 31) / 32;
    if (!upload) {
        h.cap_memories.resize(n);
        h.cap_waves.resize(3 * words);
    }
    auto move = [&](DevicePtr ptr, void *host, Count bytes) {
        s.d.check(upload ? s.d.upload(ptr, host, bytes) : s.d.download(host, ptr, bytes),
                  "transfer the lived capillary cortex and pressure field");
    };
    if (cap_appetites.ptr) {
        if (!upload)
            h.appetites.resize(n);
        if (!h.appetites.empty())
            move(cap_appetites.ptr, h.appetites.data(), n);
    }
    if (cap_contexts.ptr) {
        if (!upload)
            h.cap_histories.resize(n);
        if (!h.cap_histories.empty())
            move(cap_contexts.ptr, h.cap_histories.data(), Count(n) * sizeof(capillary::History));
    }
    if (!h.cap_memories.empty())
        move(cap_neurons.ptr, h.cap_memories.data(), Count(n) * sizeof(capillary::Mind));
    if (!h.cap_waves.empty()) {
        move(cap_board.ptr + Count(current_wave()) * words * 4, h.cap_waves.data(), Count(words) * 4);
        move(cap_board.ptr + Count(words) * 8, h.cap_waves.data() + words, Count(words) * 8);
        if (upload && !h.cap_law.mode)
            move(cap_board.ptr + Count(current_wave() ^ 1u) * words * 4, h.cap_waves.data(), Count(words) * 4);
    }
}
void extend_capillaries(Habitat &h, Word side) {
    transfer_hands(h,true);
    if (h.cap_waves.empty())
        return;
    s.d.check(s.d.upload(cap_neurons.ptr, h.cap_memories.data(), h.cap_memories.size() * sizeof(capillary::Mind)),
              "keep each growing resident's intervention contacts");
    if (cap_appetites.ptr && !h.appetites.empty())
        s.d.check(s.d.upload(cap_appetites.ptr, h.appetites.data(), h.appetites.size()),
                  "retain occupied appetitive contacts on the frontier");
    if (cap_contexts.ptr && !h.cap_histories.empty())
        s.d.check(
            s.d.upload(cap_contexts.ptr, h.cap_histories.data(), h.cap_histories.size() * sizeof(capillary::History)),
            "retain the established population's context contacts");
    Temporary prior(s.d, h.cap_waves.size() * 4);
    s.d.check(s.d.upload(prior.ptr, h.cap_waves.data(), h.cap_waves.size() * 4),
              "preserve established pressure and coatings");
    Function resize{};
    s.bind(resize, "capillary_expand", Image::floodplain);
    Word phase = current_wave(), width = c.side;
    launch(resize, (n + 127) / 128, 128, cap_board.ptr, prior.ptr, side, width, phase);
    if (!h.cap_law.mode) {
        phase ^= 1;
        launch(resize, (n + 127) / 128, 128, cap_board.ptr, prior.ptr, side, width, phase);
    }
    synchronize();
}
void capillary_report(const Habitat &h, const std::string &path) {
    if (!cap_ledgers.ptr)
        return;
    using Row = std::array<Count, sizeof(capillary::Ledger) / sizeof(Count)>;
    std::array<Row, 128> blocks{};
    s.d.check(s.d.download(blocks.data(), cap_ledgers.ptr, sizeof(blocks)), "count work and conserved carbon");
    Row total{};
    for (auto &b : blocks)
        for (Word j = 0; j < total.size(); ++j)
            total[j] += b[j];
    auto a = std::bit_cast<capillary::Ledger>(total);
    if (a.correct > a.forecast || a.changed > a.experiments || a.placed + a.lifted != a.experiments ||
        a.model_food > a.caught || a.heat != a.attended + a.experiments + a.nets + a.pumped)
        throw std::logic_error("capillary energy or material accounting disagrees");
    Count learned = 0, observed = 0, contrasts = 0;
    for (Word j = 0; j < h.cap_memories.size(); ++j)
        if (h.bodies[j].energy) {
            auto &m = h.cap_memories[j];
            learned += bool(m.tissue.on | m.tissue.off);
            observed += capillary::count(m.tissue.on | m.tissue.off);
            contrasts += bool(m.after);
        }
    std::cout << "capillary_field=" << tick << "  updates=" << a.steps << "  springs=" << a.springs
              << "  pressure=" << a.carried << "  paid_looks=" << a.attended << "  experiments=" << a.experiments
              << "  altered=" << a.changed << "  pieces_placed=" << a.placed << "  pieces_lifted=" << a.lifted
              << "  nets=" << a.nets << "  food=" << a.caught << "  forecast=" << a.forecast
              << "  correct=" << a.correct << "  transient_sensitive=" << a.model_actions
              << "  return_nets=" << a.return_nets << "  return_food=" << a.return_food
              << "  loose_stock_used=" << a.reused << "  sensitive_food=" << a.model_food << "  edits=" << a.edits
              << "  spikes=" << a.spikes << "  observed_contrasts=" << a.contrasts << "  heat=" << a.heat
              << "  living_learners=" << learned << "  living_contacts=" << observed
              << "  living_witnesses=" << contrasts << "  mode=" << h.cap_law.mode
              << "  period=" << (1u << h.cap_law.shift) << "  resident_bytes=" << sizeof(capillary::Mind)
              << "  physical_bits=4\n";
    if (h.cap_law.mode & capillary::foraging_ports) {
        Word learned = 0;
        for (Word j = 0; j < h.appetites.size(); ++j)
            learned += h.bodies[j].energy && h.appetites[j];
        std::cout << "chemical_foraging=" << tick << "  writer_visits=" << a.writing
                  << "  receiver_visits=" << a.reading << "  paid_pumps=" << a.pumped
                  << "  changed_pulses=" << a.pump_effect << "  net_choices=" << a.chemical_nets
                  << "  writer_food=" << a.writer_food << "  receiver_food=" << a.reader_food
                  << "  contact_edits=" << a.appetitive_edits << "  living_contacts=" << learned
                  << "  compared=" << a.port_compared << "  different_choices=" << a.changed_preference
                  << "  changed_pumps=" << a.changed_pumps << "  changed_nets=" << a.changed_nets
                  << "  extra_meals=" << a.port_extra_food << "  missed_meals=" << a.port_missed_food
                  << "  bytes_per_slot=1  fixed=" << bool(h.cap_law.mode & capillary::fixed_appetite) << '\n';
    }
    if (h.cap_law.mode & capillary::catalytic_surface)
        std::cout << "catalytic_field=" << tick << "  updates=" << a.steps << "  changed_material=" << a.mixed_material
                  << "  enabled_pulses=" << a.pulse_added << "  prevented_pulses=" << a.pulse_removed
                  << "  switched_sills=" << a.switched_sills << "  self_closing=" << a.one_shot
                  << "  enabled_grips=" << a.active_grips << "  enabled_food=" << a.enabled_bite
                  << "  forgone_food=" << a.foregone_bite << "  extra_site_bits=0\n";
    if (cap_experiences.ptr) {
        if (a.caught + a.food_forgone != a.other_food + a.food_added)
            throw std::logic_error("the two immediate food accounts disagree");
        std::cout << "context_choices=" << tick << "  compared=" << a.comparisons
                  << "  changed_hand=" << a.different_hand << "  changed_net=" << a.different_net
                  << "  actual_food=" << a.caught << "  pooled_food=" << a.other_food << "  extra=" << a.food_added
                  << "  lost=" << a.food_forgone << "  actual_heat=" << a.heat << "  pooled_heat=" << a.other_cost
                  << "  observed_sites=" << capillary_field.context_sites << '\n';
    }
    if (cap_contexts.ptr) {
        Count occupied = 0, anchors = 0;
        for (Word i = 0; i < h.cap_histories.size(); ++i)
            if (h.bodies[i].energy) {
                occupied += bool(capillary::footprint(h.cap_histories[i]));
                anchors += bool(h.cap_histories[i].recent & 128);
            }
        std::cout << "context_field=" << tick << "  bytes=" << sizeof(capillary::History) << "  residents=" << occupied
                  << "  with_anchor=" << anchors << "  ambiguous_looks=" << a.ambiguity_seen
                  << "  checks_due=" << a.tests_due << "  physical_checks=" << a.rechecked
                  << "  diagnostic_updates=" << a.retained_tests << "  resolved=" << a.read_context
                  << "  connected=" << bool(h.cap_law.mode & capillary::contextual) << '\n';
    }
    std::cout << "capillary_dispatch=" << (cap_scheduled ? "scheduled" : "full")
              << "  pressure_launches=" << cap_launches << "  cargo_launches=" << (cargo_totals.ptr ? cap_launches : 0)
              << "  graph_frames=" << cap_graph_nodes << "  ticks=" << tick - cap_start
              << "  physical_beats=" << a.steps / n << '\n';
    if (!path.empty()) {
        std::vector<capillary::Note> notes(capillary_field.samples);
        s.d.check(s.d.download(notes.data(), cap_notes.ptr, notes.size() * sizeof(notes[0])),
                  "read real pressure and grip outcomes");
        EvidenceFile file(path);
        file.out << "tick\tsite\tslot\tborn\trock\tprevious\tsensed\tactual\tfired\thand\tforecast\tcast\tfood_"
                    "before\tfood\tafter_food\tenergy_before\tenergy_after\theat\tcargo_before\tcargo_after\tedge_"
                    "before\tedge_after\twater\tlaw\tdendrites\tmotor\tbefore_on\tbefore_"
                    "off\ton\toff\tincoming\tpad\toutgoing\tcoating\tentry_head\twants_"
                    "flow\tfilter\tdonation\tcontext\tpassive_before\trecent_before\tpassive_after\trecent_"
                    "after\tcontext_alternatives\tpermission\trecheck_due\tresolved";
        if (h.cap_law.mode & capillary::catalytic_surface)
            file.out << "\tgeology\tcatalytic";
        if (h.cap_law.mode & capillary::foraging_ports)
            file.out << "\tport\tlight\tchoice\tpump\tbasal_row\tappetite_before\tappetite_after\tappetite_"
                        "edits\tappetite_rule\tappetite_audit\tunlearned_choice\tunlearned_cast\tunlearned_food\tstock_"
                        "threshold\tnet_available\tmouthful\tpump_available";
        file.out << '\n';
        for (auto &v : notes)
            if (v.tick) {
                file.out << v.tick;
                for (Count x : std::initializer_list<Count>{v.site,
                                                            v.slot,
                                                            v.born,
                                                            v.rock,
                                                            v.previous,
                                                            v.sensed,
                                                            v.actual,
                                                            v.fired,
                                                            v.hand,
                                                            v.forecast,
                                                            v.cast,
                                                            v.food_before,
                                                            v.food,
                                                            v.after_food,
                                                            v.energy_before,
                                                            v.after_energy,
                                                            v.heat,
                                                            v.cargo_before,
                                                            v.cargo_after,
                                                            v.edge_before,
                                                            v.edge_after,
                                                            v.wet,
                                                            v.law,
                                                            v.dendrites,
                                                            v.motor,
                                                            v.prior.on,
                                                            v.prior.off,
                                                            v.learned.on,
                                                            v.learned.off,
                                                            v.incoming,
                                                            v.pad,
                                                            v.outgoing,
                                                            v.deposited,
                                                            v.threshold,
                                                            v.wants_flow,
                                                            v.filter,
                                                            v.donation,
                                                            v.in_context,
                                                            capillary::footprint(v.context_before),
                                                            v.context_before.recent,
                                                            capillary::footprint(v.context_after),
                                                            v.context_after.recent,
                                                            v.alternatives,
                                                            v.memory_gate,
                                                            v.review,
                                                            v.chosen_context})
                    file.out << '\t' << x;
                if (h.cap_law.mode & capillary::catalytic_surface)
                    file.out << '\t' << v.geology << '\t' << v.chemistry;
                if (h.cap_law.mode & capillary::foraging_ports)
                    for (Word z : {v.port, v.light, v.choice, v.pump, v.basal_row, v.contacts_before, v.contacts_after,
                                   v.food_edits, v.food_law, v.appetite_audit, v.prior_choice, v.shadow_cast,
                                   v.shadow_food, v.stock_threshold, v.net_available, v.mouthful, v.pump_available})
                        file.out << '\t' << z;
                file.out << '\n';
            }
        file.publish();
    }
}
void trace_capillary_wake(const Options &o) {
    auto film = capillary_field.film;
    if (!film.marks)
        throw std::invalid_argument("record a running capillary field to trace a pulse");
    Word distance = std::min(o.cap_delay, c.side - 1), width = film.side, area = width * width, frames = film.frames,
         shift = capillary_field.law.shift, mask = (1u << shift) - 1;
    Word end = (Count(tick) + mask) >> shift, initial = (Count(cap_start) + mask) >> shift,
         begin = std::max(initial, end > frames ? end - frames : 0u);
    Word span = end - begin, whole = width == c.side;
    EvidenceFile f(o.cap_film);
    f.out << "source_epoch\torigin\twidth\tworld\torigin_x\torigin_y\tdepth\tdelta_"
             "x\tplace\tslot\tborn\tphysical\tmaterial\trock\thand\tnatural\tspring\tebb\tnet\tstock\tfood\tpossible\tp"
             "laced\tlifted\tmodel_sensitive\treturn_net\n";
    if (span <= distance) {
        f.publish();
        std::cout << "capillary_wakes=0  frames=" << span << "  candidate_cells=0\n";
        return;
    }
    Function enumerate{}, replay{};
    s.bind(enumerate, "capillary_donors", Image::floodplain);
    s.bind(replay, "capillary_wakes", Image::floodplain);
    Temporary count(s.d, 4);
    s.d.check(s.d.zero(count.ptr, 0, 4), "count changed pressure sources");
    DevicePtr none = 0;
    Count opportunities = Count(span - distance) * area;
    if (opportunities > ~0u)
        throw std::invalid_argument("the single recording exceeds 32-bit spatial indexing");
    launch(enumerate, (opportunities + 127) / 128, 128, cap_archive.ptr, width, frames, begin, span, distance,
           count.ptr, none);
    synchronize();
    Word n = 0;
    s.d.check(s.d.download(&n, count.ptr, 4), "size the sparse intervention list");
    Count enabled = 0, blocked = 0, other = 0, prevented = 0, winners = 0;
    Word furthest = 0;
    capillary::Donor source_event{};
    capillary::Mark handler{};
    capillary::Echo meal{};
    if (n) {
        Temporary donors(s.d, Count(n) * sizeof(capillary::Donor)), results(s.d, Count(n) * sizeof(capillary::Echo));
        s.d.check(s.d.zero(count.ptr, 0, 4), "index material sources");
        launch(enumerate, (opportunities + 127) / 128, 128, cap_archive.ptr, width, frames, begin, span, distance,
               count.ptr, donors.ptr);
        launch(replay, (n + 63) / 64, 64, cap_archive.ptr, width, frames, whole, distance, donors.ptr, n, results.ptr);
        synchronize();
        std::vector<capillary::Donor> ids(n);
        std::vector<capillary::Echo> effects(n);
        s.d.check(s.d.download(ids.data(), donors.ptr, ids.size() * sizeof(ids[0])),
                  "read identified material handlers");
        s.d.check(s.d.download(effects.data(), results.ptr, effects.size() * sizeof(effects[0])),
                  "read matched downstream effects");
        std::vector<Word> permutation(n);
        std::iota(permutation.begin(), permutation.end(), 0);
        std::ranges::sort(permutation, [&](auto a, auto b) {
            return std::tie(ids[a].moment, ids[a].position) < std::tie(ids[b].moment, ids[b].position);
        });
        std::vector<capillary::Donor> sorted(n);
        for (Word i = 0; i < n; ++i)
            sorted[i] = ids[permutation[i]];
        s.d.check(s.d.upload(donors.ptr, sorted.data(), Count(n) * sizeof(sorted[0])),
                  "gather only the recorded intervention cones");
        Word samples = (distance + 1) * (distance + 2) / 2, capacity = std::min(n, 1024u);
        Temporary compact(s.d, Count(samples) * capacity * sizeof(capillary::Mark));
        Function gather{};
        s.bind(gather, "capillary_fragments", Image::floodplain);
        std::vector<capillary::Mark> marks(Count(samples) * capacity);
        for (Word start = 0; start < n; start += capacity) {
            Word batch = std::min(capacity, n - start);
            DevicePtr source = donors.ptr + Count(start) * sizeof(sorted[0]);
            launch(gather, (batch * samples + 127) / 128, 128, cap_archive.ptr, width, frames, distance, source, batch,
                   compact.ptr);
            synchronize();
            s.d.check(s.d.download(marks.data(), compact.ptr, Count(samples) * batch * sizeof(marks[0])),
                      "read the sparse pressure cones");
            for (Word rank = 0; rank < batch; ++rank) {
                Word i = permutation[start + rank];
                auto root = ids[i];
                auto effect = effects[i];
                enabled += effect.enabled;
                blocked += effect.suppressed;
                other += effect.other;
                prevented += effect.prevented;
                winners += bool(effect.other);
                if (effect.horizon > furthest) {
                    furthest = effect.horizon;
                    source_event = root;
                    handler = marks[rank * samples];
                    meal = effect;
                }
                for (Word d = 0; d <= distance; ++d)
                    for (Word dx = 0; dx <= d; ++dx) {
                        Word px = root.position % width + dx, py = root.position / width + d - dx;
                        if (!whole && (px >= width || py >= width))
                            continue;
                        Word at = (py % width) * width + (px % width);
                        auto m = marks[rank * samples + d * (d + 1) / 2 + dx];
                        f.out << root.moment << '\t' << root.position << '\t' << width << '\t' << c.side << '\t'
                              << film.x << '\t' << film.y << '\t' << d << '\t' << dx << '\t' << at << '\t' << m.slot
                              << '\t' << m.born << '\t' << (m.signals & 63) << '\t' << ((m.signals >> 6) & 63) << '\t'
                              << ((m.signals >> 12) & 31) << '\t' << ((m.signals >> 17) & 7) << '\t'
                              << ((m.signals >> 20) & 1) << '\t' << ((m.signals >> 21) & 1) << '\t'
                              << ((m.signals >> 22) & 1) << '\t' << ((m.signals >> 23) & 1) << '\t'
                              << (m.resources & 65535) << '\t' << ((m.resources >> 16) & 15) << '\t'
                              << ((m.resources >> 20) & 15) << '\t' << ((m.resources >> 28) & 1) << '\t'
                              << ((m.resources >> 29) & 1) << '\t' << ((m.signals >> 25) & 1) << '\t'
                              << ((m.signals >> 26) & 1) << '\n';
                    }
            }
        }
    }
    if (furthest) {
        auto root = source_event.position;
        Word dx = (meal.site % width + width - root % width) % width;
        std::cout << "capillary_neighbour=" << source_event.moment << "  donor=" << handler.slot
                  << "  born=" << handler.born << "  recipient=" << meal.slot << "  recipient_born=" << meal.born
                  << "  donor_x=" << ((film.x + root % width) % c.side)
                  << "  donor_y=" << ((film.y + root / width) % c.side) << "  pulse_steps=" << furthest << "  dx=" << dx
                  << "  dy=" << furthest - dx << "  hand=" << ((handler.signals >> 17) & 7) << "  food=" << meal.net
                  << "  tick_period=" << (1u << shift);
        for (Word j = 0; j <= furthest; ++j)
            std::cout << "  wave" << j << "=" << meal.influence[j];
        std::cout << '\n';
    }
    f.publish();
    std::cout << "capillary_wakes=" << n << "  frames=" << span << "  candidate_cells=" << opportunities
              << "  enabled=" << enabled << "  suppressed=" << blocked
              << "  distinct_interventions_feeding_others=" << winners << "  attributed_food=" << other
              << "  prevented_food=" << prevented << "  horizon=" << furthest << "  window=" << width
              << "  fixed_observed_chemistry=" << bool(capillary_field.law.mode & capillary::catalytic_surface)
              << "  replay_depth=" << distance << "  requested_depth=" << o.cap_delay
              << "  downloaded_marks=" << Count(n) * (distance + 1) * (distance + 2) / 2
              << "  recording_bytes=" << Count(frames) * area * sizeof(capillary::Mark) << '\n';
}

void context_events(const std::string &path) {
    Word sites = capillary_field.context_sites;
    std::vector<capillary::ContextNote> evidence(2 * sites);
    s.d.check(s.d.download(evidence.data(), cap_experiences.ptr, evidence.size() * sizeof(evidence[0])),
              "read natural checks, decisions and independently forecast food");
    EvidenceFile f(path);
    f.out << "food_sample\ttick\tsite\tslot\tborn\trock\tgene\tmotor\tcoin\toptions\trows\twant\tvaluable\tnet_"
             "available\teyes\tlimbs\tstock\tenergy\thand\tpooled_hand\tcast\tpooled_cast\trow\tpooled_"
             "row\tfood\tpooled_food\theat\tpooled_heat\tpermission\treview\tresolved\tsigned\ton\toff\tpassive_"
             "before\trecent_before\tpassive_after\trecent_after\talternatives";
    if (capillary_field.law.mode & capillary::foraging_ports)
        f.out << "\tport\tport_choice\tpump";
    f.out << '\n';
    const capillary::ContextNote *sample = nullptr;
    Word top = 0;
    for (Word j = 0; j < evidence.size(); ++j) {
        auto &n = evidence[j];
        if (!n.tick)
            continue;
        Word rank = n.food > n.other_food ? 4 : n.food < n.other_food ? 3 : n.hand != n.other_hand ? 2 : 1;
        if (rank > top || (rank == top && n.tick < sample->tick)) {
            sample = &n;
            top = rank;
        }
        f.out << Word(j >= sites);
        for (Count a : std::initializer_list<Count>{n.tick,           n.site,
                                                    n.slot,           n.born,
                                                    n.rock,           n.gene,
                                                    n.motor,          n.coin,
                                                    n.options,        n.rows,
                                                    n.want,           n.valuable,
                                                    n.net_available,  n.eyes,
                                                    n.limbs,          n.stock,
                                                    n.energy,         n.hand,
                                                    n.other_hand,     n.cast,
                                                    n.other_cast,     n.row,
                                                    n.other_row,      n.food,
                                                    n.other_food,     n.heat,
                                                    n.other_heat,     n.permission,
                                                    n.review,         n.chosen,
                                                    n.signed_contact, n.tissue.on,
                                                    n.tissue.off,     capillary::footprint(n.before),
                                                    n.before.recent,  capillary::footprint(n.after),
                                                    n.after.recent,   n.diagnostic})
            f.out << '\t' << a;
        if (capillary_field.law.mode & capillary::foraging_ports)
            f.out << '\t' << n.port << '\t' << n.choice << '\t' << n.pump;
        f.out << '\n';
    }
    f.publish();
    if (sample) {
        auto &a = *sample;
        std::cout << "context_scene=" << a.tick << "  site=" << a.site << "  slot=" << a.slot << "  born=" << a.born
                  << "  rock=" << a.rock << "  pooled_hand=" << a.other_hand << "  actual_hand=" << a.hand
                  << "  pooled_food=" << a.other_food << "  actual_net=" << a.cast << "  pooled_net=" << a.other_cast
                  << "  want=" << a.want << "  actual_food=" << a.food << "  stock=" << a.stock << "  cost=" << a.heat
                  << "  other_cost=" << a.other_heat << "  pulse=" << a.row << "  alternative=" << a.other_row
                  << "  present=" << capillary::footprint(a.before) << "  receipts=" << (a.before.recent & 0xffffff)
                  << "  signed=" << a.signed_contact << '\n';
    }
}

void catalytic_proof(const std::string &path) {
    Function check{};
    s.bind(check, "flood_catalytic_contacts", Image::floodplain);
    constexpr Word cases = 32 * 4 * 4 * 64;
    Temporary receipts(s.d, Count(cases) * sizeof(Word));
    FieldTimer time(s.d);
    time.start(stream);
    launch(check, cases / 128, 128, receipts.ptr);
    time.stop(stream);
    synchronize();
    std::vector<Word> all(cases);
    s.d.check(s.d.download(all.data(), receipts.ptr, Count(cases) * sizeof(Word)),
              "read every two-beat catalytic contact");
    EvidenceFile out(path);
    out.out << "geology\tbound\tcarrier\tinput\tlaw\tsecond_law\tflow\tsecond_flow\tfirst_carrier\tfirst_bound\tlast_"
               "carrier\tlast_bound\n";
    Word close = 0, shift = 0, repeat = 0;
    for (Word n = 0; n < cases; ++n) {
        auto r = all[n];
        Word first = (r >> 10) & 1, second = (r >> 11) & 1;
        close += first && !second;
        repeat += first && second;
        shift += (r & 31) != ((r >> 5) & 31);
        out.out << (n >> 10);
        for (Word v : {(n >> 8) & 3, (n >> 6) & 3, n & 63, r & 31, (r >> 5) & 31, first, second, (r >> 12) & 3,
                       (r >> 14) & 3, (r >> 16) & 3, (r >> 18) & 3})
            out.out << '\t' << v;
        out.out << '\n';
    }
    out.publish();
    std::cout << "catalytic_contacts=" << cases << "  state_changes=" << shift << "  self_closing=" << close
              << "  repeated_flow=" << repeat << "  cuda_ms=" << time.milliseconds() << '\n';
}

void catalytic_closures(const Options &o) {
    auto film = capillary_field.film;
    Word mask = (1u << capillary_field.law.shift) - 1, shift = capillary_field.law.shift;
    Word end = (Count(tick) + mask) >> shift, start = (Count(cap_start) + mask) >> shift;
    Word begin = std::max(start, end > film.frames ? end - film.frames : 0u), span = end - begin,
         area = film.side * film.side;
    EvidenceFile file(o.catalytic_film);
    file.out << "epoch\tplace\twidth\tx\ty\trow\tgeology\tcarrier\tbound\tnext_carrier\tnext_bound\tfirst_flow\tsecond_"
                "flow\tfirst_actor\tfirst_born\tfirst_hand\tsecond_actor\tsecond_born\tsecond_hand\n";
    Word amount = 0, handled = 0;
    Count opportunities = span > 1 ? Count(span - 1) * area : 0;
    if (opportunities) {
        Function sample{};
        s.bind(sample, "catalytic_closures", Image::floodplain);
        Temporary count(s.d, 4);
        s.d.check(s.d.zero(count.ptr, 0, 4), "clear the closure counter");
        DevicePtr none = 0;
        launch(sample, (opportunities + 127) / 128, 128, film, begin, span, count.ptr, none);
        synchronize();
        s.d.check(s.d.download(&amount, count.ptr, 4), "count observed changes at conserved pigment pairs");
        if (amount) {
            Temporary captures(s.d, Count(amount) * sizeof(capillary::Closing));
            s.d.check(s.d.zero(count.ptr, 0, 4), "rewind the closure counter");
            launch(sample, (opportunities + 127) / 128, 128, film, begin, span, count.ptr, captures.ptr);
            synchronize();
            std::vector<capillary::Closing> records(amount);
            s.d.check(s.d.download(records.data(), captures.ptr, Count(amount) * sizeof(records[0])),
                      "read only observed consecutive closures");
            std::sort(records.begin(), records.end(), [](const auto &a, const auto &b) {
                return std::tie(a.epoch, a.place) < std::tie(b.epoch, b.place);
            });
            for (auto &r : records) {
                auto a = r.first, b = r.second;
                Word hand = (a.signals >> 17) & 7;
                handled += hand != 0;
                file.out << r.epoch;
                for (Word v : {r.place, film.side, (film.x + r.place % film.side) & (c.side - 1),
                               (film.y + r.place / film.side) & (c.side - 1), (a.signals >> 6) & 63, a.signals >> 27,
                               (a.resources >> 24) & 3, a.resources >> 30, (b.resources >> 24) & 3, b.resources >> 30,
                               (a.signals >> 20) & 1, (b.signals >> 20) & 1, a.slot, a.born, hand, b.slot, b.born,
                               (b.signals >> 17) & 7})
                    file.out << '\t' << v;
                file.out << '\n';
            }
        }
    }
    file.publish();
    std::cout << "catalytic_window=" << tick << "  frames=" << span << "  width=" << film.side
              << "  adjacent_pairs=" << opportunities << "  witnessed_closures=" << amount << "  handled=" << handled
              << "  raw_bytes=" << Count(film.frames) * area * sizeof(capillary::Mark)
              << "  downloaded_bytes=" << 4 + Count(amount) * sizeof(capillary::Closing) << '\n';
}

void chemical_receptions(const std::string &path) {
    auto view = capillary_field.gallery;
    capillary::Deliveries t{};
    s.d.check(s.d.download(&t, cap_deliveries.ptr, sizeof(t)), "count pressure pulses reaching another animal");
    Word n = std::min<Count>(t.pairs, view.capacity);
    std::vector<capillary::Reception> records(n);
    if (n)
        s.d.check(s.d.download(records.data(), cap_readers.ptr, Count(n) * sizeof(records[0])),
                  "read the natural donor and independent receiver");
    std::sort(records.begin(), records.end(), [](auto &a, auto &b) {
        return std::tie(a.tick, a.destination, a.line) < std::tie(b.tick, b.destination, b.line);
    });
    EvidenceFile file(path);
    file.out << "tick\tworld\tsource\tdestination\twriter\twriter_born\treader\treader_born\twriter_input\twriter_"
                "without\twriter_rock\twriter_learned\taxis\tinput\talternative\trock\tsensed\tother_"
                "sensed\tmove\tother_move\tno_contacts\tother_no_contacts\tcast\tother_"
                "cast\twants\tavailable\tsorting\tstock\tmouthful\tenergy\theat\tfood\tother_food\n";
    const capillary::Reception *best = nullptr;
    auto rank = [](const auto &v) {
        return std::tuple{bool(v.writer_data & (1u << 18)) && v.move != v.other_move && v.bare == v.other_bare,
                          v.move != v.other_move, v.food > v.other_food, bool(v.writer_data & (1u << 18)),
                          v.light != v.other_light};
    };
    for (auto &r : records) {
        if (!best || rank(r) > rank(*best))
            best = &r;
        file.out << r.tick;
        for (Word v : {c.side,
                       r.source,
                       r.destination,
                       r.writer,
                       r.writer_born,
                       r.reader,
                       r.reader_born,
                       (r.writer_data >> 7) & 63,
                       (r.writer_data >> 1) & 63,
                       (r.writer_data >> 13) & 31,
                       (r.writer_data >> 18) & 1,
                       r.line,
                       r.input,
                       r.counterfactual,
                       r.rock,
                       r.light,
                       r.other_light,
                       r.move,
                       r.other_move,
                       r.bare,
                       r.other_bare,
                       r.cast,
                       r.other_cast,
                       r.wants,
                       r.available,
                       r.sorting,
                       r.stock,
                       r.mouthful,
                       r.energy,
                       r.heat,
                       r.food,
                       r.other_food})
            file.out << '\t' << v;
        file.out << '\n';
    }
    file.publish();
    if (best) {
        auto &r = *best;
        std::cout << "chemical_neighbour=" << r.tick << "  writer=" << r.writer << "  writer_born=" << r.writer_born
                  << "  reader=" << r.reader << "  reader_born=" << r.reader_born << "  source=" << r.source
                  << "  destination=" << r.destination << "  axis=" << r.line
                  << "  writer_learned=" << ((r.writer_data >> 18) & 1) << "  movement=" << r.move
                  << "  without_pump=" << r.other_move << "  cold=" << r.bare << "  cold_without=" << r.other_bare
                  << "  food=" << r.food << "  alternative_food=" << r.other_food
                  << "  delay=" << (1u << capillary_field.law.shift) << '\n';
    }
    std::cout << "chemical_receptions=" << tick << "  window=" << view.side << "  encounters=" << t.pairs
              << "  changed_light=" << t.sole_source << "  changed_move=" << t.changed_move
              << "  learned_pump=" << t.acquired_source << "  learned_response=" << t.acquired_response
              << "  learned_both=" << t.learned_both << "  extra_food=" << t.gained << "  forgone_food=" << t.missed
              << "  sampled=" << n
              << "  source_bytes=" << Count(view.side) * view.side * 2 * sizeof(capillary::PumpOrigin)
              << "  downloaded_bytes=" << sizeof(t) + Count(n) * sizeof(capillary::Reception) << '\n';
}
